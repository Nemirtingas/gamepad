#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <mutex>

#include <Foundation/NSString.h>
#include <IOKit/hid/IOHIDLib.h>

#define GAMEPAD_BUTTON_APPLE_A 1
#define GAMEPAD_BUTTON_APPLE_B 2
#define GAMEPAD_BUTTON_APPLE_X 4
#define GAMEPAD_BUTTON_APPLE_Y 5
#define GAMEPAD_BUTTON_APPLE_LEFTSHOULDER 7
#define GAMEPAD_BUTTON_APPLE_RIGHTSHOULDER 8
#define GAMEPAD_BUTTON_APPLE_START 12
#define GAMEPAD_BUTTON_APPLE_HOME 13
#define GAMEPAD_BUTTON_APPLE_LEFTTHUMB 14
#define GAMEPAD_BUTTON_APPLE_RIGHTTHUMB 15
#define GAMEPAD_BUTTON_APPLE_SELECT 548

#define GAMEPAD_AXIS_APPLE_LEFTTHUMBX  kHIDUsage_GD_X
#define GAMEPAD_AXIS_APPLE_LEFTTHUMBY  kHIDUsage_GD_Y
#define GAMEPAD_AXIS_APPLE_RIGHTTHUMBX kHIDUsage_GD_Z
#define GAMEPAD_AXIS_APPLE_RIGHTTHUMBY kHIDUsage_GD_Rz
#define GAMEPAD_AXIS_APPLE_LEFTBUMPER  kHIDUsage_Sim_Brake
#define GAMEPAD_AXIS_APPLE_RIGHTBUMPER kHIDUsage_Sim_Accelerator

#define GAMEPAD_AXIS_X360_LEFTTHUMBX  kHIDUsage_GD_X
#define GAMEPAD_AXIS_X360_LEFTTHUMBY  kHIDUsage_GD_Y
#define GAMEPAD_AXIS_X360_RIGHTTHUMBX kHIDUsage_GD_Rx
#define GAMEPAD_AXIS_X360_RIGHTTHUMBY kHIDUsage_GD_Ry
#define GAMEPAD_AXIS_X360_LEFTBUMPER  kHIDUsage_GD_Z
#define GAMEPAD_AXIS_X360_RIGHTBUMPER kHIDUsage_GD_Rz

class IOSGamepad
{
public:
    struct GamepadButton
    {
        IOHIDElementRef Handle;
        uint32_t Type;
    };

    struct GamepadHat
    {
        IOHIDElementRef Handle;
        long Min;
        long Max;
    };

    struct GamepadAxis
    {
        IOHIDElementRef Handle;
        float Min;
        float Max;
        float NormalizedMin;
        float NormalizedMax;
        uint32_t Type;
    };

public:
    IOHIDDeviceRef _DeviceHandle;
    // XBox One Wireless controller:
    // A : 1
    // B : 2
    // X : 4
    // Y : 5
    // L1: 7
    // R1: 8
    // START: 12
    // XBOX: 13
    // L3: 14
    // R3: 15
    // SELECT: 548
    std::vector<GamepadButton> _Buttons;
    std::vector<GamepadAxis> _Axis;
    std::vector<GamepadHat> _Hats;

    void AddElements(CFArrayRef elements, std::set<IOHIDElementCookie>& cookies)
    {
        for (int i = 0; i < CFArrayGetCount(elements); i++)
        {
            IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);

            const uint32_t type = IOHIDElementGetType(element);

            switch (type)
            {
                case kIOHIDElementTypeCollection:
                    AddElements(IOHIDElementGetChildren(element), cookies);
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
                // Left Thumb X Axis on Xbox360 driver
                // Left Thumb X Axis on Apple's driver
                case kHIDUsage_GD_X:
                    _Axis.emplace_back(GamepadAxis{
                        element,
                        (float)IOHIDElementGetLogicalMin(element),
                        (float)IOHIDElementGetLogicalMax(element),
                        -1.0f,
                        1.0f,
                        usage
                    });
                    break;

                // Left Thumb Y Axis on Xbox360 driver
                // Left Thumb Y Axis on Apple's driver
                case kHIDUsage_GD_Y:
                    _Axis.emplace_back(GamepadAxis{
                        element,
                        (float)IOHIDElementGetLogicalMax(element),
                        (float)IOHIDElementGetLogicalMin(element),
                        -1.0f,
                        1.0f,
                        usage
                    });
                    break;

                // Left Bumper Axis on Xbox360 driver
                // Right Thumb X Axis on Apple's driver
                case kHIDUsage_GD_Z:
                    _Axis.emplace_back(GamepadAxis{
                        element,
                        (float)IOHIDElementGetLogicalMin(element),
                        (float)IOHIDElementGetLogicalMax(element),
                        -1.0f,
                        1.0f,
                        usage
                    });
                    break;

                // Right Thumb X Axis on Xbox360 driver
                // Unused on Apple's driver
                case kHIDUsage_GD_Rx:
                    break;

                // Right Thumb Y Axis on Xbox360 driver
                // Unused on Apple's driver
                case kHIDUsage_GD_Ry:
                    break;

                // Right Bumper X Axis on Xbox360 driver
                // Right Thumb X Axis on Apple's driver
                case kHIDUsage_GD_Rz:
                    _Axis.emplace_back(GamepadAxis{
                        element,
                        (float)IOHIDElementGetLogicalMin(element),
                        (float)IOHIDElementGetLogicalMax(element),
                        0.0f,
                        1.0f,
                        usage
                    });
                    break;

                // Unused Xbox360 driver
                // Right Bumper on Apple's driver
                case kHIDUsage_Sim_Accelerator:
                    _Axis.emplace_back(GamepadAxis{
                        element,
                        (float)IOHIDElementGetLogicalMin(element),
                        (float)IOHIDElementGetLogicalMax(element),
                        0.0f,
                        1.0f,
                        usage
                    });
                    break;

                // Unused Xbox360 driver
                // Left Bumper on Apple's driver
                case kHIDUsage_Sim_Brake:
                    _Axis.emplace_back(GamepadAxis{
                        element,
                        (float)IOHIDElementGetLogicalMin(element),
                        (float)IOHIDElementGetLogicalMax(element),
                        0.0f,
                        1.0f,
                        usage
                    });
                    break;

                // Hat on Xbox360 driver
                // Hat on Apple's driver
                case kHIDUsage_GD_Hatswitch:
                    _Hats.emplace_back(GamepadHat{ element, (long)IOHIDElementGetLogicalMin(element), (long)IOHIDElementGetLogicalMax(element) });
                    break;

                // Other Axis
                case kHIDUsage_GD_Slider:
                case kHIDUsage_GD_Dial:
                case kHIDUsage_GD_Wheel:
                case kHIDUsage_Sim_Rudder:
                case kHIDUsage_Sim_Throttle:
                    _Axis.emplace_back(GamepadAxis{
                        element,
                        (float)IOHIDElementGetLogicalMin(element),
                        (float)IOHIDElementGetLogicalMax(element),
                        -1.0f,
                        1.0f,
                        usage
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
                    _Buttons.emplace_back(GamepadButton{ element, usage });
                    break;

                default:
                    if (type == kIOHIDElementTypeInput_Button)
                    {
                        _Buttons.emplace_back(GamepadButton{ element, usage });
                        break;
                    }

                    const uint32_t usage_page = IOHIDElementGetUsagePage(element);

                    if (usage_page == kHIDPage_Button || usage_page == kHIDPage_Consumer)
                    {
                        _Buttons.emplace_back(GamepadButton{ element, usage });
                        break;
                    }

                    if (type == kIOHIDElementTypeInput_Axis)
                    {// TODO:
                    }

                    break;
            }
        }
    }

public:
    void CloseDevice()
    {
        if(_DeviceHandle != nullptr)
        {

            _DeviceHandle = nullptr;
        }
    }

    bool SetupGamepad(IOHIDDeviceRef device_handle)
    {
        CloseDevice();

        _DeviceHandle = device_handle;
        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(_DeviceHandle, nullptr, kIOHIDOptionsTypeNone);
        std::set<IOHIDElementCookie> known_cookies;


        AddElements(elements, known_cookies);

        uint32_t vendorId = 0;
        uint32_t productId = 0;

        CFNumberRef vendor = static_cast<CFNumberRef>(IOHIDDeviceGetProperty(_DeviceHandle, CFSTR(kIOHIDVendorIDKey)));
        if (vendor) CFNumberGetValue(vendor, kCFNumberSInt32Type, &vendorId);
        CFNumberRef product = static_cast<CFNumberRef>(IOHIDDeviceGetProperty(_DeviceHandle, CFSTR(kIOHIDProductIDKey)));
        if (product) CFNumberGetValue(product, kCFNumberSInt32Type, &productId);

        // Force Feedback
        //FFCAPABILITIES ff_caps;
        //if (SUCCEEDED(
        //              ForceFeedback::FFDeviceAdapter::Create(IOHIDDeviceGetService(m_device), &m_ff_device)) &&
        //    SUCCEEDED(FFDeviceGetForceFeedbackCapabilities(m_ff_device->m_device, &ff_caps)))
        //{
        //    InitForceFeedback(m_ff_device, ff_caps.numFfAxes);
        //}

        return true;
    }

    bool GetButton()
    {
        //long name = IOHIDElementGetUsage(button);
        //IOHIDValueRef hid_value;
        //long value = 0;
        //if (IOHIDDeviceGetValue(gamepad._DeviceHandle, button, &hid_value) == kIOReturnSuccess)
        //    value = IOHIDValueGetIntegerValue(hid_value);
        //
        //std::cout << "Button " << name << ": " << value << std::endl;

        return false;
    }

    int GetHat()
    {
        //IOHIDValueRef hid_value;
        //
        //if (IOHIDDeviceGetValue(gamepad._DeviceHandle, hat, &hid_value) == kIOReturnSuccess)
        //{
        //    long position = IOHIDValueGetIntegerValue(hid_value);
        //    long min = IOHIDElementGetLogicalMin(hat);
        //    long max = IOHIDElementGetLogicalMax(hat);
        //
        //    if (position >= min && position <= max)
        //    {
        //        // normalize the position
        //        position -= min;
        //
        //        switch (position)
        //        {
        //            case 0:
        //                std::cout << "Hat UP" << std::endl;
        //                break;
        //            case 1:
        //                std::cout << "Hat UP RIGHT" << std::endl;
        //                break;
        //            case 2:
        //                std::cout << "Hat RIGHT" << std::endl;
        //                break;
        //            case 3:
        //                std::cout << "Hat DOWN RIGHT" << std::endl;
        //                break;
        //            case 4:
        //                std::cout << "Hat DOWN" << std::endl;
        //                break;
        //            case 5:
        //                std::cout << "Hat DOWN LEFT" << std::endl;
        //                break;
        //            case 6:
        //                std::cout << "Hat LEFT" << std::endl;
        //                break;
        //            case 7:
        //                std::cout << "Hat UP LEFT" << std::endl;
        //                break;
        //        }
        //    }
        //    else
        //    {
        //        std::cout << "Hat NONE" << std::endl;
        //    }
        //}

        return 0;
    }
};

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

constexpr CFTimeInterval FOREVER = 1e20;
static std::mutex s_hotplug_mutex;
static std::thread s_hotplug_thread;
static RunLoopHelper s_stopper;
static IOHIDManagerRef HIDManager = nullptr;
static CFStringRef OurRunLoop = CFSTR("GamepadOSXLoop");

static std::vector<IOSGamepad> s_gamepads;

static std::string GetDeviceRefName(IOHIDDeviceRef inIOHIDDeviceRef)
{
    const NSString* name = reinterpret_cast<const NSString*>(
                                                             IOHIDDeviceGetProperty(inIOHIDDeviceRef, CFSTR(kIOHIDProductKey)));
    return (name != nullptr) ? std::string([name UTF8String]) : "Unknown device";
}

static void DeviceRemovalCallback(void* inContext, IOReturn inResult, void* inSender,
                                  IOHIDDeviceRef inIOHIDDeviceRef)
{
    std::lock_guard<std::mutex> lk(s_hotplug_mutex);
    for( auto it = s_gamepads.begin(); it != s_gamepads.end(); ++it )
    {
        if (it->_DeviceHandle == inIOHIDDeviceRef)
        {
            std::cout << "Gamepad disconnected" << std::endl;
            s_gamepads.erase(it);
            break;
        }
    }
}

static void DeviceMatchingCallback(void* inContext, IOReturn inResult, void* inSender,
                                   IOHIDDeviceRef inIOHIDDeviceRef)
{
    std::string name = GetDeviceRefName(inIOHIDDeviceRef);

    // Add a device if it's of a type we want
    if (IOHIDDeviceConformsTo(inIOHIDDeviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick) ||
        IOHIDDeviceConformsTo(inIOHIDDeviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad) ||
        IOHIDDeviceConformsTo(inIOHIDDeviceRef, kHIDPage_GenericDesktop,
                              kHIDUsage_GD_MultiAxisController))
    {
        IOSGamepad gamepad;
        if(gamepad.SetupGamepad(inIOHIDDeviceRef))
        {
            std::lock_guard<std::mutex> lk(s_hotplug_mutex);
            std::cout << "Gamepad connected" << std::endl;
            s_gamepads.emplace_back(std::move(gamepad));
        }
    }
}

void Init()
{
    HIDManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!HIDManager)
    {
        //ERROR_LOG_FMT(CONTROLLERINTERFACE, "Failed to create HID Manager reference");
    }

    IOHIDManagerSetDeviceMatching(HIDManager, nullptr);
    if (IOHIDManagerOpen(HIDManager, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
    {
        //ERROR_LOG_FMT(CONTROLLERINTERFACE, "Failed to open HID Manager");
    }

    // Callbacks for acquisition or loss of a matching device
    IOHIDManagerRegisterDeviceMatchingCallback(HIDManager, DeviceMatchingCallback, nullptr);
    IOHIDManagerRegisterDeviceRemovalCallback(HIDManager, DeviceRemovalCallback, nullptr);

    // Match devices that are plugged in right now
    IOHIDManagerScheduleWithRunLoop(HIDManager, CFRunLoopGetCurrent(), OurRunLoop);
    while (CFRunLoopRunInMode(OurRunLoop, 0, TRUE) == kCFRunLoopRunHandledSource)
    {
    };
    IOHIDManagerUnscheduleFromRunLoop(HIDManager, CFRunLoopGetCurrent(), OurRunLoop);

    // Enable hotplugging
    s_hotplug_thread = std::thread([] {
        IOHIDManagerScheduleWithRunLoop(HIDManager, CFRunLoopGetCurrent(), OurRunLoop);
        s_stopper.AddToRunLoop(CFRunLoopGetCurrent(), OurRunLoop);
        CFRunLoopRunInMode(OurRunLoop, FOREVER, FALSE);
        s_stopper.RemoveFromRunLoop(CFRunLoopGetCurrent(), OurRunLoop);
        IOHIDManagerUnscheduleFromRunLoop(HIDManager, CFRunLoopGetCurrent(), OurRunLoop);
    });
}

void DeInit()
{
    if (HIDManager)
    {
        s_stopper.Signal();
        s_hotplug_thread.join();

        // This closes all devices as well
        IOHIDManagerClose(HIDManager, kIOHIDOptionsTypeNone);
        CFRelease(HIDManager);
        HIDManager = nullptr;
    }
}

constexpr inline float normalize_value(float min, float max, float value)
{
    return (value - min) / (max - min);
}

constexpr inline float denormalize_value(float min, float max, float value)
{
    return value * (max - min) + min;
}

constexpr inline float rerange_value(float src_min, float src_max, float dst_min, float dst_max, float value)
{
    return denormalize_value(dst_min, dst_max,
                             normalize_value(src_min, src_max, value));
}

int main(int argc, char *argv[])
{
    //InitializeJoysticks();

    Init();

    std::stringstream sstr;
    while(1)
    {
        {
            std::lock_guard<std::mutex> lk(s_hotplug_mutex);

            sstr << "\x1B[H\x1B[2J";
            sstr << "\x1b[1;1f";

            for(auto& gamepad: s_gamepads)
            {
                for(auto& button: gamepad._Buttons)
                {
                    IOHIDValueRef hid_value;
                    long value = 0;
                    if (IOHIDDeviceGetValue(gamepad._DeviceHandle, button.Handle, &hid_value) == kIOReturnSuccess)
                        value = IOHIDValueGetIntegerValue(hid_value);

                    sstr << "Button " << button.Type << ": " << value << "\n";
                }

                for(auto& hat: gamepad._Hats)
                {
                    IOHIDValueRef hid_value;
                    if (IOHIDDeviceGetValue(gamepad._DeviceHandle, hat.Handle, &hid_value) == kIOReturnSuccess)
                    {
                        long position = IOHIDValueGetIntegerValue(hid_value);

                        if (position >= hat.Min && position <= hat.Max)
                        {
                            // normalize the position
                            position -= hat.Min;

                            switch (position)
                            {
                                case 0:
                                    sstr << "Hat UP" << "\n";
                                    break;
                                case 1:
                                    sstr << "Hat UP RIGHT" << "\n";
                                    break;
                                case 2:
                                    sstr << "Hat RIGHT" << "\n";
                                    break;
                                case 3:
                                    sstr << "Hat DOWN RIGHT" << "\n";
                                    break;
                                case 4:
                                    sstr << "Hat DOWN" << "\n";
                                    break;
                                case 5:
                                    sstr << "Hat DOWN LEFT" << "\n";
                                    break;
                                case 6:
                                    sstr << "Hat LEFT" << "\n";
                                    break;
                                case 7:
                                    sstr << "Hat UP LEFT" << "\n";
                                    break;
                            }
                        }
                        else
                        {
                            sstr << "Hat NONE" << "\n";
                        }
                    }
                }

                for(auto& axis: gamepad._Axis)
                {
                    IOHIDValueRef value;
                    if (IOHIDDeviceGetValue(gamepad._DeviceHandle, axis.Handle, &value) == kIOReturnSuccess)
                    {
                        // IOHIDValueGetIntegerValue() crashes when trying
                        // to convert unusually large element values.
                        if (IOHIDValueGetLength(value) > 2)
                            return 0;

                        float position = IOHIDValueGetIntegerValue(value);

                        sstr << "Axis " << axis.Type << ": " << position << " <-> " << rerange_value(axis.Min, axis.Max, axis.NormalizedMin, axis.NormalizedMax, position) << "\n";
                    }
                }
            }
        }
        std::cout << sstr.str() << std::endl;
        sstr.str(std::string());
        usleep(100000);
    }

    DeInit();

    return 0;
}