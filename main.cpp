#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <CoreFoundation/CoreFoundation.h>

#include <iostream>
#include <iomanip>

static 	IOHIDManagerRef hid_mgr = 0x0;

static void hid_device_removal_callback(void *context, IOReturn result,
                                        void *sender, IOHIDDeviceRef hid_ref)
{
}

static int init_hid_manager(void)
{

	/* Initialize all the HID Manager Objects */
	hid_mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
	if (hid_mgr)
    {
		IOHIDManagerSetDeviceMatching(hid_mgr, NULL);
		IOHIDManagerScheduleWithRunLoop(hid_mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		IOHIDManagerRegisterDeviceRemovalCallback(hid_mgr, hid_device_removal_callback, NULL);
		return 0;
	}
	
	return -1;
}

static int hid_init(void)
{
	if (!hid_mgr)
    {
		return init_hid_manager();
	}
	
	/* Already initialized. */
	return 0;
}

static int hid_exit(void)
{
	if (hid_mgr)
    {
		/* Close the HID manager. */
		IOHIDManagerClose(hid_mgr, kIOHIDOptionsTypeNone);
		CFRelease(hid_mgr);
		hid_mgr = NULL;
	}
	
	return 0;
}

static void process_pending_events()
{
	SInt32 res;
	do
    {
		res = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.001, FALSE);
	}
    while(res != kCFRunLoopRunFinished && res != kCFRunLoopRunTimedOut);
}

static int32_t get_int_property(IOHIDDeviceRef device, CFStringRef key)
{
	CFTypeRef ref;
	int32_t value;
	
	ref = IOHIDDeviceGetProperty(device, key);
	if (ref) {
		if (CFGetTypeID(ref) == CFNumberGetTypeID()) {
			CFNumberGetValue((CFNumberRef) ref, kCFNumberSInt32Type, &value);
			return value;
		}
	}
	return 0;
}

static unsigned short get_vendor_id(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDVendorIDKey));
}

static unsigned short get_product_id(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDProductIDKey));
}


static int32_t get_max_report_length(IOHIDDeviceRef device)
{
	return get_int_property(device, CFSTR(kIOHIDMaxInputReportSizeKey));
}

static int get_string_property(IOHIDDeviceRef device, CFStringRef prop, wchar_t *buf, size_t len)
{
	CFStringRef str;
	
	if (!len)
		return 0;
	
	str = (CFStringRef)IOHIDDeviceGetProperty(device, prop);
	
	buf[0] = 0;
	
	if (str) {
		len --;
		
		CFIndex str_len = CFStringGetLength(str);
		CFRange range;
		range.location = 0;
		range.length = (str_len > len)? len: str_len;
		CFIndex used_buf_len;
		CFIndex chars_copied;
		chars_copied = CFStringGetBytes(str,
										range,
										kCFStringEncodingUTF32LE,
										(char)'?',
										FALSE,
										(UInt8*)buf,
										len,
										&used_buf_len);
		
		buf[chars_copied] = 0;
		return (int)chars_copied;
	}
	else
		return 0;
	
}

static int get_string_property_utf8(IOHIDDeviceRef device, CFStringRef prop, char *buf, size_t len)
{
	CFStringRef str;
	if (!len)
		return 0;
	
	str = (CFStringRef)IOHIDDeviceGetProperty(device, prop);
	
	buf[0] = 0;
	
	if (str) {
		len--;
		
		CFIndex str_len = CFStringGetLength(str);
		CFRange range;
		range.location = 0;
		range.length = (str_len > len)? len: str_len;
		CFIndex used_buf_len;
		CFIndex chars_copied;
		chars_copied = CFStringGetBytes(str,
										range,
										kCFStringEncodingUTF8,
										(char)'?',
										FALSE,
										(UInt8*)buf,
										len,
										&used_buf_len);
		
		buf[chars_copied] = 0;
		return (int)used_buf_len;
	}
	else
		return 0;
}


static int get_serial_number(IOHIDDeviceRef device, wchar_t *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDSerialNumberKey), buf, len);
}

static int get_manufacturer_string(IOHIDDeviceRef device, wchar_t *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDManufacturerKey), buf, len);
}

static int get_product_string(IOHIDDeviceRef device, wchar_t *buf, size_t len)
{
	return get_string_property(device, CFSTR(kIOHIDProductKey), buf, len);
}

static int make_path(IOHIDDeviceRef device, char *buf, size_t len)
{
	int res;
	unsigned short vid, pid;
	char transport[32];
	
	buf[0] = '\0';
	
	res = get_string_property_utf8(
								   device, CFSTR(kIOHIDTransportKey),
								   transport, sizeof(transport));
	
	if (!res)
		return -1;
	
	vid = get_vendor_id(device);
	pid = get_product_id(device);
	
	res = snprintf(buf, len, "%s_%04hx_%04hx_%p",
				   transport, vid, pid, device);
	
	
	buf[len-1] = '\0';
	return res+1;
}

static void hid_enumerate()
{
	CFIndex num_devices;
	int i;
	
	/* Set up the HID Manager if it hasn't been done */
	if (hid_init() < 0)
		return;
	
	/* give the IOHIDManager a chance to update itself */
	process_pending_events();
	
	/* Get a list of the Devices */
	CFSetRef device_set = IOHIDManagerCopyDevices(hid_mgr);
	if (!device_set)
		return;

	/* Convert the list into a C array so we can iterate easily. */	
	num_devices = CFSetGetCount(device_set);
	if (!num_devices) {
		CFRelease(device_set);
		return;
	}
	IOHIDDeviceRef *device_array = (IOHIDDeviceRef*)calloc(num_devices, sizeof(IOHIDDeviceRef));
	CFSetGetValues(device_set, (const void **) device_array);
	
	/* Iterate over each device, making an entry for it. */	
	for (i = 0; i < num_devices; i++)
    {
		unsigned short vendor_id;
		unsigned short product_id;
#define BUF_LEN 256
		wchar_t buf[BUF_LEN];
		char cbuf[BUF_LEN];
		
		IOHIDDeviceRef dev = device_array[i];
		
		if (!dev)
			continue;

		vendor_id  = get_vendor_id(dev);
		product_id = get_product_id(dev);
		
		// Get the Usage Page and Usage for this device.
		int usage_page = get_int_property(dev, CFSTR(kIOHIDPrimaryUsagePageKey));
		int usage = get_int_property(dev, CFSTR(kIOHIDPrimaryUsageKey));
			
		/* Fill out the record */
		make_path(dev, cbuf, sizeof(cbuf));
        std::string device_path = cbuf;
			
		/* Serial Number */
		get_serial_number(dev, buf, BUF_LEN);
		std::wstring serial_number = buf;
			
		/* Manufacturer and Product strings */
		get_manufacturer_string(dev, buf, BUF_LEN);
		std::wstring manufacturer_string = buf;
		get_product_string(dev, buf, BUF_LEN);
		std::wstring product_string = buf;
			
		/* Release Number */
		int release_number = get_int_property(dev, CFSTR(kIOHIDVersionNumberKey));

        std::cout  << "device path : " << device_path         << std::endl;
        std::wcout << std::hex << std::showbase
                   << "vendor_id   : " << vendor_id           << std::endl
                   << "product_id  : " << product_id          << std::endl
                   << "usage_page  : " << usage_page          << std::endl
                   << "usage       : " << usage               << std::endl
                   << "serial num  : " << serial_number       << std::endl
                   << "manufacturer: " << manufacturer_string << std::endl
                   << "product     : " << product_string      << std::endl
                   << std::endl;
	}
	
	free(device_array);
	CFRelease(device_set);
}

int main(int argc, char *argv[])
{
    std::cout << init_hid_manager() << std::endl;
    
    hid_enumerate();

    return 0;
}
