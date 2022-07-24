#include "gamepad.cpp"

namespace gamepad {

class RunLoopHelper
{
    CFRunLoopSourceRef _LoopSource;
    CFRunLoopRef _RunLoop = nullptr;

public:
    RunLoopHelper()
    {
        CFRunLoopSourceContext ctx{};
        ctx.version = 0;
        ctx.perform = [](void*) {
            CFRunLoopStop(CFRunLoopGetCurrent());
        };
        _LoopSource = CFRunLoopSourceCreate(kCFAllocatorDefault, 0, &ctx);
    }

    ~RunLoopHelper()
    {
        CFRelease(_LoopSource);
    }

    void Signal()
    {
        CFRunLoopSourceSignal(_LoopSource);
        if (_RunLoop != nullptr)
            CFRunLoopWakeUp(_RunLoop);
    }

    void AddToRunLoop(CFRunLoopRef runloop, CFStringRef mode)
    {
        _RunLoop = runloop;
        CFRunLoopAddSource(runloop, _LoopSource, mode);
    }

    void RemoveFromRunLoop(CFRunLoopRef runloop, CFStringRef mode)
    {
        _RunLoop = nullptr;
        CFRunLoopRemoveSource(runloop, _LoopSource, mode);
    }
};

constexpr CFTimeInterval TimeInfinite = 1e20;
static std::thread s_hotplug_thread;
static RunLoopHelper s_stopper;
static CFStringRef OurRunLoop = CFSTR("GamepadOSXLoop");
static IOHIDManagerRef HIDManager = nullptr;

struct button_def_t
{
    IOHIDElementRef handle;
    uint32_t button_id;
};

struct axis_def_t
{
    IOHIDElementRef handle;
    uint32_t axis_id;
    float min;
    float max;
};

struct button_t
{
    IOHIDElementRef handle;
    uint32_t button_value;
};

struct hat_t
{
    IOHIDElementRef handle;
    long min;
    long max;
};

struct axis_t
{
    IOHIDElementRef handle;
    float min;
    float max;
    float normalized_min;
    float normalized_max;
    float* mapped_value;
};

struct gamepad_context_t
{
    IOHIDDeviceRef device_handle;
    bool dead;

    std::vector<button_t> buttons;
    std::vector<axis_t> axis;
    hat_t hat;

    gamepad_id_t gamepad_id;
    gamepad_state_t gamepad_state;
};

static void parse_gamepad_elements(CFArrayRef elements, std::set<IOHIDElementCookie>& cookies, std::vector<button_def_t>& buttons, std::vector<axis_def_t>& axis, std::vector<hat_t>& hats)
{
    for (int i = 0; i < CFArrayGetCount(elements); i++)
    {
        IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);

        const uint32_t type = IOHIDElementGetType(element);

        switch (type)
        {
            case kIOHIDElementTypeCollection:
                parse_gamepad_elements(IOHIDElementGetChildren(element), cookies, buttons, axis, hats);
                continue;
            case kIOHIDElementTypeOutput:
                continue;
        }

        IOHIDElementCookie cookie = IOHIDElementGetCookie(element);

        // This element has already been parsed
        if (cookies.count(cookie) > 0)
            continue;

        cookies.insert(cookie);

        const uint32_t usage = IOHIDElementGetUsage(element);

        switch (usage)
        {
            case kHIDUsage_GD_X:
            case kHIDUsage_GD_Y:
            case kHIDUsage_GD_Z:
            case kHIDUsage_GD_Rx:
            case kHIDUsage_GD_Ry:
            case kHIDUsage_GD_Rz:
            case kHIDUsage_Sim_Accelerator:
            case kHIDUsage_Sim_Brake:
            // Other potential axis
            //case kHIDUsage_GD_Slider:
            //case kHIDUsage_GD_Dial:
            //case kHIDUsage_GD_Wheel:
            //case kHIDUsage_Sim_Rudder:
            //case kHIDUsage_Sim_Throttle:
                axis.emplace_back(axis_def_t{
                    element,
                    usage,
                    (float)IOHIDElementGetLogicalMin(element),
                    (float)IOHIDElementGetLogicalMax(element)
                });
                break;

            case kHIDUsage_GD_Hatswitch:
                hats.emplace_back(hat_t{
                    element,
                    (long)IOHIDElementGetLogicalMin(element),
                    (long)IOHIDElementGetLogicalMax(element)
                });
                break;

            // Buttons
            case kHIDUsage_GD_DPadUp:
            case kHIDUsage_GD_DPadDown:
            case kHIDUsage_GD_DPadRight:
            case kHIDUsage_GD_DPadLeft:
            case kHIDUsage_GD_Start:
            case kHIDUsage_GD_Select:
            case kHIDUsage_GD_SystemMainMenu:
                buttons.emplace_back(button_def_t{ element, usage });
                break;

            default:
                if (type == kIOHIDElementTypeInput_Button)
                {
                    buttons.emplace_back(button_def_t{ element, usage });
                    break;
                }

                const uint32_t usage_page = IOHIDElementGetUsagePage(element);

                if (usage_page == kHIDPage_Button || usage_page == kHIDPage_Consumer)
                {
                    buttons.emplace_back(button_def_t{ element, usage });
                    break;
                }

                if (type == kIOHIDElementTypeInput_Axis)
                {// TODO:
                }

                break;
        }
    }
}

static int32_t get_gamepad_infos(gamepad_context_t* p_context)
{
    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(p_context->device_handle, nullptr, kIOHIDOptionsTypeNone);
    std::set<IOHIDElementCookie> known_cookies;
    std::vector<button_def_t> buttons_definitions;
    std::vector<axis_def_t> axis_definitions;
    std::vector<hat_t> hats_definitions;
    uint32_t axis_mode = 0;
    uint32_t button_mode = 0;

    uint32_t vendorId = 0xffffffff;
    uint32_t productId = 0xffffffff;

    CFNumberRef vendor = static_cast<CFNumberRef>(IOHIDDeviceGetProperty(p_context->device_handle, CFSTR(kIOHIDVendorIDKey)));
    if (vendor)
        CFNumberGetValue(vendor, kCFNumberSInt32Type, &vendorId);

    CFNumberRef product = static_cast<CFNumberRef>(IOHIDDeviceGetProperty(p_context->device_handle, CFSTR(kIOHIDProductIDKey)));
    if (product)
        CFNumberGetValue(product, kCFNumberSInt32Type, &productId);

    p_context->gamepad_id.vendorID = vendorId;
    p_context->gamepad_id.productID = productId;

    parse_gamepad_elements(elements, known_cookies, buttons_definitions, axis_definitions, hats_definitions);

    for (auto& item : axis_definitions)
    {
        switch(item.axis_id)
        {
            case kHIDUsage_GD_X           : axis_mode |= 0x01; break;
            case kHIDUsage_GD_Y           : axis_mode |= 0x02; break;
            case kHIDUsage_GD_Rx          : axis_mode |= 0x04; break;
            case kHIDUsage_GD_Ry          : axis_mode |= 0x08; break;
            case kHIDUsage_GD_Z           : axis_mode |= 0x10; break;
            case kHIDUsage_GD_Rz          : axis_mode |= 0x20; break;
            case kHIDUsage_Sim_Brake      : axis_mode |= 0x40; break;
            case kHIDUsage_Sim_Accelerator: axis_mode |= 0x80; break;
        }
    }

    if (axis_mode == 0x3f)
    {// XUSB kHIDUsage_GD_X, kHIDUsage_GD_Y, kHIDUsage_GD_Rx, kHIDUsage_GD_Ry, kHIDUsage_GD_Z, kHIDUsage_GD_Rz mode
        // This mode seems to be used when the gamepad is wired.
        for (auto& item : axis_definitions)
        {
            switch(item.axis_id)
            {
                case kHIDUsage_GD_X:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.min   , // axis_min
                        item.max   , // axis_max
                        -1.0f      , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.left_stick.x // mapped_value
                    });
                    break;

                case kHIDUsage_GD_Y:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.max   , // axis_min
                        item.min   , // axis_max
                        -1.0f      , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.left_stick.y // mapped_value
                    });
                    break;

                case kHIDUsage_GD_Rx:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.min   , // axis_min
                        item.max   , // axis_max
                        -1.0f      , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.right_stick.x // mapped_value
                    });
                    break;

                case kHIDUsage_GD_Ry:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.max   , // axis_min
                        item.min   , // axis_max
                        -1.0f      , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.right_stick.y // mapped_value
                    });
                    break;

                case kHIDUsage_GD_Z:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.min   , // axis_min
                        item.max   , // axis_max
                        0.0f       , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.left_trigger // mapped_value
                    });
                    break;

                case kHIDUsage_GD_Rz:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.min   , // axis_min
                        item.max   , // axis_max
                        0.0f       , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.right_trigger // mapped_value
                    });
                    break;
            }
        }
    }
    else if(axis_mode == 0xf3)
    {// XUSB kHIDUsage_GD_X, kHIDUsage_GD_Y, kHIDUsage_GD_Z, kHIDUsage_GD_Rz, kHIDUsage_Sim_Accelerator, kHIDUsage_Sim_Brake mode
        // This mode seems to be used when the gamepad is wireless.
        axis_t* device_axis;
        for (auto& item : axis_definitions)
        {
            switch(item.axis_id)
            {
                case kHIDUsage_GD_X:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.min   , // axis_min
                        item.max   , // axis_max
                        -1.0f      , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.left_stick.x // mapped_value
                    });
                    break;

                case kHIDUsage_GD_Y:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.max   , // axis_min
                        item.min   , // axis_max
                        -1.0f      , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.left_stick.y // mapped_value
                    });
                    break;

                case kHIDUsage_GD_Z:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.min   , // axis_min
                        item.max   , // axis_max
                        -1.0f      , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.right_stick.x // mapped_value
                    });
                    break;

                case kHIDUsage_GD_Rz:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.max   , // axis_min
                        item.min   , // axis_max
                        -1.0f      , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.right_stick.y // mapped_value
                    });
                    break;

                case kHIDUsage_Sim_Brake:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.min   , // axis_min
                        item.max   , // axis_max
                        0.0f       , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.left_trigger // mapped_value
                    });
                    break;

                case kHIDUsage_Sim_Accelerator:
                    p_context->axis.emplace_back(axis_t{
                        item.handle, // axis_handle
                        item.min   , // axis_min
                        item.max   , // axis_max
                        0.0f       , // normalized_min
                        1.0f       , // normalized_max
                        &p_context->gamepad_state.right_trigger // mapped_value
                    });
                    break;
            }
        }
    }
    else
    {
        return gamepad::failed;
    }

    for (auto& item : buttons_definitions)
    {
        switch(item.button_id)
        {
            case 1  : button_mode |= 0x0001; break;
            case 2  : button_mode |= 0x0002; break;
            case 3  : button_mode |= 0x0004; break;
            case 4  : button_mode |= 0x0008; break;
            case 5  : button_mode |= 0x0010; break;
            case 6  : button_mode |= 0x0020; break;
            case 7  : button_mode |= 0x0040; break;
            case 8  : button_mode |= 0x0080; break;
            case 9  : button_mode |= 0x0100; break;
            case 10 : button_mode |= 0x0200; break;
            case 548: button_mode |= 0x0400; break;
        }
    }

    if (button_mode == 0x03ff)
    {
        for (auto const& button_def : buttons_definitions)
        {
            switch(button_def.button_id)
            {
                case 1:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_a  // button_value
                    });
                    break;

                case 2:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_b  // button_value
                    });
                    break;

                case 3:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_x  // button_value
                    });
                    break;

                case 4:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_y  // button_value
                    });
                    break;

                case 5:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_left_shoulder  // button_value
                    });
                    break;

                case 6:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_right_shoulder  // button_value
                    });
                    break;

                case 7:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_left_thumb  // button_value
                    });
                    break;

                case 8:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_right_thumb  // button_value
                    });
                    break;

                case 9:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_start  // button_value
                    });
                    break;

                case 10:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_back  // button_value
                    });
                    break;

                case 11:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_guide  // button_value
                    });
                    break;

                case 12:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_up  // button_value
                    });
                    break;

                case 13:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_down  // button_value
                    });
                    break;

                case 14:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_left  // button_value
                    });
                    break;

                case 15:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_right  // button_value
                    });
                    break;

            }
        }
    }
    else if (button_mode == 0x07ff)
    {
        for (auto const& button_def : buttons_definitions)
        {
            switch(button_def.button_id)
            {
                case 1:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_a  // button_value
                    });
                    break;

                case 2:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_b  // button_value
                    });
                    break;

                case 4:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_x  // button_value
                    });
                    break;

                case 5:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_y  // button_value
                    });
                    break;

                case 7:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_left_shoulder  // button_value
                    });
                    break;

                case 8:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_right_shoulder  // button_value
                    });
                    break;

                case 12:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_start  // button_value
                    });
                    break;

                case 13:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_guide  // button_value
                    });
                    break;

                case 14:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_left_thumb  // button_value
                    });
                    break;

                case 15:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_right_thumb  // button_value
                    });
                    break;

                case 548:
                    p_context->buttons.emplace_back(button_t{
                        button_def.handle, // button_handle
                        gamepad::button_back  // button_value
                    });
                    break;

            }
        }
    }
    else
    {
        return gamepad::failed;
    }

    if(!hats_definitions.empty())
        p_context->hat = hats_definitions[0];

    return gamepad::success;
}

static void internal_free_context(gamepad_context_t** pp_context)
{
    if (*pp_context == nullptr)
        return;

    delete *pp_context;
    *pp_context = nullptr;
}

static int32_t internal_create_context(gamepad_context_t** pp_context, IOHIDDeviceRef device_handle)
{
    //std::string name = GetDeviceRefName(device_handle);
    *pp_context = new gamepad_context_t;

    if (*pp_context == nullptr)
        return gamepad::failed;

    (*pp_context)->device_handle = device_handle;
    (*pp_context)->dead = false;
    memset(&(*pp_context)->gamepad_state, 0, sizeof(gamepad_state_t));
    memset(&(*pp_context)->hat, 0, sizeof(hat_t));

    return get_gamepad_infos(*pp_context);
}

static void device_removal_callback(void* inContext, IOReturn inResult, void* inSender, IOHIDDeviceRef inIOHIDDeviceRef)
{
    std::lock_guard<std::mutex> lk(s_gamepad_mutex);
    for (int i = 0; i < max_connected_gamepads; ++i)
    {
        if (s_gamepads[i]->device_handle == inIOHIDDeviceRef)
        {
            internal_free_context(&s_gamepads[i]);
            break;
        }
    }
}

static void device_matching_callback(void* inContext, IOReturn inResult, void* inSender,
                                   IOHIDDeviceRef inIOHIDDeviceRef)
{
    std::lock_guard<std::mutex> lk(s_gamepad_mutex);

    // Add a device if it's of a type we want
    if (IOHIDDeviceConformsTo(inIOHIDDeviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick) ||
        IOHIDDeviceConformsTo(inIOHIDDeviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad) ||
        IOHIDDeviceConformsTo(inIOHIDDeviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_MultiAxisController))
    {
        for (int i = 0; i < max_connected_gamepads; ++i)
        {
            if (s_gamepads[i] == nullptr || s_gamepads[i]->dead)
            {
                internal_create_context(&s_gamepads[i], inIOHIDDeviceRef);
                break;
            }
        }
    }
}

static int32_t setup_hid_manager()
{
    if(HIDManager != nullptr)
        return gamepad::success;

    HIDManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (HIDManager == nullptr)
    {
        return gamepad::failed;
    }

    IOHIDManagerSetDeviceMatching(HIDManager, nullptr);
    if (IOHIDManagerOpen(HIDManager, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
    {
        // HIDManagerOpen might fail, its not a real problem, seems to work even if it failed.
        //CFRelease(HIDManager);
        //HIDManager = nullptr;
        //return gamepad::failed;
    }

    // Callbacks for new or removal of a device
    IOHIDManagerRegisterDeviceMatchingCallback(HIDManager, device_matching_callback, nullptr);
    IOHIDManagerRegisterDeviceRemovalCallback(HIDManager, device_removal_callback, nullptr);

    s_hotplug_thread = std::thread([]()
    {
        IOHIDManagerScheduleWithRunLoop(HIDManager, CFRunLoopGetCurrent(), OurRunLoop);
        s_stopper.AddToRunLoop(CFRunLoopGetCurrent(), OurRunLoop);
        CFRunLoopRunInMode(OurRunLoop, TimeInfinite, FALSE);
        s_stopper.RemoveFromRunLoop(CFRunLoopGetCurrent(), OurRunLoop);
        IOHIDManagerUnscheduleFromRunLoop(HIDManager, CFRunLoopGetCurrent(), OurRunLoop);
   });

    return gamepad::success;
}

static inline void set_button_value(uint32_t& buttons, uint32_t value, bool activated)
{
    if (activated)
        buttons |= value;
    else
        buttons &= ~value;
}

static int32_t internal_get_gamepad(uint32_t index, gamepad_context_t** pp_context)
{
    // setup hid manager only in internal_get_gamepad, because this function is always called before any other gamepad function.
    if (setup_hid_manager() != gamepad::success)
        return gamepad::failed;

    if (s_gamepads[index] != nullptr && !s_gamepads[index]->dead)
    {
        *pp_context = s_gamepads[index];
        return gamepad::success;
    }

    return gamepad::failed;
}

static int32_t internal_update_gamepad_state(gamepad_context_t* p_context)
{
    IOHIDValueRef value;

    if(p_context->hat.handle != nullptr)
    {
        if (IOHIDDeviceGetValue(p_context->device_handle, p_context->hat.handle, &value) != kIOReturnSuccess)
        {
            p_context->dead = true;
            return gamepad::failed;
        }

        long position = IOHIDValueGetIntegerValue(value);

        if (position >= p_context->hat.min && position <= p_context->hat.max)
        {
            // normalize the position
            position -= p_context->hat.min;

            switch (position)
            {
                case 0:
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_up, true);
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_down | gamepad::button_left | gamepad::button_right, false);
                    break;

                case 1:
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_up | gamepad::button_right, true);
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_down | gamepad::button_left, false);
                    break;

                case 2:
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_right, true);
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_up | gamepad::button_down | gamepad::button_left, false);
                    break;

                case 3:
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_down | gamepad::button_right, true);
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_up | gamepad::button_left, false);
                    break;

                case 4:
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_down, true);
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_up | gamepad::button_left | gamepad::button_right, false);
                    break;

                case 5:
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_down | gamepad::button_left, true);
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_up | gamepad::button_right, false);
                    break;

                case 6:
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_left, true);
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_up | gamepad::button_down | gamepad::button_right, false);
                    break;

                case 7:
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_up | gamepad::button_left, true);
                    set_button_value(p_context->gamepad_state.buttons, gamepad::button_down | gamepad::button_right, false);
                    break;
            }
        }
        else
        {
            set_button_value(p_context->gamepad_state.buttons, gamepad::button_up | gamepad::button_down | gamepad::button_left | gamepad::button_right, false);
        }
    }

    for (auto const& button : p_context->buttons)
    {
        long int_value = 0;
        if (IOHIDDeviceGetValue(p_context->device_handle, button.handle, &value) != kIOReturnSuccess)
        {
            p_context->dead = true;
            return gamepad::failed;
        }

        int_value = IOHIDValueGetIntegerValue(value);
        set_button_value(p_context->gamepad_state.buttons, button.button_value, int_value);
    }

    for (auto const& axis : p_context->axis)
    {
        if (IOHIDDeviceGetValue(p_context->device_handle, axis.handle, &value) == kIOReturnSuccess)
        {
            // IOHIDValueGetIntegerValue() crashes when trying
            // to convert unusually large element values.
            if (IOHIDValueGetLength(value) > 2)
            {
                p_context->dead = true;
                return gamepad::failed;
            }

            *axis.mapped_value = rerange_value(axis.min, axis.max, axis.normalized_min, axis.normalized_max, static_cast<float>(IOHIDValueGetIntegerValue(value)));
        }
        else
        {
            p_context->dead = true;
            return gamepad::failed;
        }
    }

    return gamepad::success;
}

static int32_t internal_get_gamepad_state(gamepad_context_t* p_context, gamepad_state_t* p_gamepad_state)
{
    memcpy(p_gamepad_state, &p_context->gamepad_state, sizeof(gamepad_state_t));
    return gamepad::success;
}

static int32_t internal_get_gamepad_id(gamepad_context_t* p_context, gamepad_id_t* p_gamepad_id)
{
    memcpy(p_gamepad_id, &p_context->gamepad_id, sizeof(gamepad_id_t));
    return gamepad::success;
}

static int32_t internal_set_gamepad_vibration(gamepad_context_t* p_context, float left_strength, float right_strength)
{
    return gamepad::failed;
}

static int32_t internal_set_gamepad_led(gamepad_context_t* p_context, uint8_t r, uint8_t g, uint8_t b)
{
    return gamepad::failed;
}

static void internal_free_all_contexts()
{
    for (uint32_t i = 0; i < gamepad::max_connected_gamepads; ++i)
    {
        internal_free_context(&s_gamepads[i]);
    }

    if (HIDManager != nullptr)
    {
        s_stopper.Signal();
        s_hotplug_thread.join();

        // This closes all devices as well
        IOHIDManagerClose(HIDManager, kIOHIDOptionsTypeNone);
        CFRelease(HIDManager);
        HIDManager = nullptr;
    }
}

}//namespace gamepad