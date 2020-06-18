#include <array>
#include <algorithm>
#include <cassert>
#include "Report.h"
#include "../hidpp10/Error.h"
#include "../hidpp20/Error.h"

using namespace logid::backend::hidpp;
using namespace logid::backend;

/* Report descriptors were sourced from cvuchener/hidpp */
static const std::array<uint8_t, 22> ShortReportDesc = {
        0xA1, 0x01,		// Collection (Application)
        0x85, 0x10,		//   Report ID (16)
        0x75, 0x08,		//   Report Size (8)
        0x95, 0x06,		//   Report Count (6)
        0x15, 0x00,		//   Logical Minimum (0)
        0x26, 0xFF, 0x00,	//   Logical Maximum (255)
        0x09, 0x01,		//   Usage (0001 - Vendor)
        0x81, 0x00,		//   Input (Data, Array, Absolute)
        0x09, 0x01,		//   Usage (0001 - Vendor)
        0x91, 0x00,		//   Output (Data, Array, Absolute)
        0xC0			// End Collection
};

static const std::array<uint8_t, 22> LongReportDesc = {
        0xA1, 0x01,		// Collection (Application)
        0x85, 0x11,		//   Report ID (17)
        0x75, 0x08,		//   Report Size (8)
        0x95, 0x13,		//   Report Count (19)
        0x15, 0x00,		//   Logical Minimum (0)
        0x26, 0xFF, 0x00,	//   Logical Maximum (255)
        0x09, 0x02,		//   Usage (0002 - Vendor)
        0x81, 0x00,		//   Input (Data, Array, Absolute)
        0x09, 0x02,		//   Usage (0002 - Vendor)
        0x91, 0x00,		//   Output (Data, Array, Absolute)
        0xC0			// End Collection
};

/* Alternative versions from the G602 */
static const std::array<uint8_t, 22> ShortReportDesc2 = {
        0xA1, 0x01,		// Collection (Application)
        0x85, 0x10,		//   Report ID (16)
        0x95, 0x06,		//   Report Count (6)
        0x75, 0x08,		//   Report Size (8)
        0x15, 0x00,		//   Logical Minimum (0)
        0x26, 0xFF, 0x00,	//   Logical Maximum (255)
        0x09, 0x01,		//   Usage (0001 - Vendor)
        0x81, 0x00,		//   Input (Data, Array, Absolute)
        0x09, 0x01,		//   Usage (0001 - Vendor)
        0x91, 0x00,		//   Output (Data, Array, Absolute)
        0xC0			// End Collection
};

static const std::array<uint8_t, 22> LongReportDesc2 = {
        0xA1, 0x01,		// Collection (Application)
        0x85, 0x11,		//   Report ID (17)
        0x95, 0x13,		//   Report Count (19)
        0x75, 0x08,		//   Report Size (8)
        0x15, 0x00,		//   Logical Minimum (0)
        0x26, 0xFF, 0x00,	//   Logical Maximum (255)
        0x09, 0x02,		//   Usage (0002 - Vendor)
        0x81, 0x00,		//   Input (Data, Array, Absolute)
        0x09, 0x02,		//   Usage (0002 - Vendor)
        0x91, 0x00,		//   Output (Data, Array, Absolute)
        0xC0			// End Collection
};

uint8_t hidpp::getSupportedReports(std::vector<uint8_t>&& rdesc)
{
    uint8_t ret = 0;

    auto it = std::search(rdesc.begin(), rdesc.end(), ShortReportDesc.begin(), ShortReportDesc.end());
    if(it == rdesc.end())
        it = std::search(rdesc.begin(), rdesc.end(), ShortReportDesc2.begin(), ShortReportDesc2.end());
    if(it != rdesc.end())
        ret |= HIDPP_REPORT_SHORT_SUPPORTED;

    it = std::search(rdesc.begin(), rdesc.end(), LongReportDesc.begin(), LongReportDesc2.end());
    if(it == rdesc.end())
        it = std::search(rdesc.begin(), rdesc.end(), LongReportDesc2.begin(), LongReportDesc2.end());
    if(it != rdesc.end())
        ret |= HIDPP_REPORT_LONG_SUPPORTED;

    return ret;
}

const char *Report::InvalidReportID::what() const noexcept
{
    return "Invalid report ID";
}

const char *Report::InvalidReportLength::what() const noexcept
{
    return "Invalid report length";
}

Report::Report(Report::Type type, DeviceIndex device_index,
        uint8_t feature_index, uint8_t function, uint8_t sw_id)
{
    assert(function <= functionMask);
    assert(sw_id <= swIdMask);

    switch(type)
    {
        case Type::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case Type::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            throw InvalidReportID();
    }

    _data[Offset::Type] = type;
    _data[Offset::DeviceIndex] = device_index;
    _data[Offset::Feature] = feature_index;
    _data[Offset::Function] = (function & functionMask) << 4 | (sw_id & swIdMask);
}

Report::Report(const std::vector<uint8_t>& data)
{
    _data = data;

    switch(_data[Offset::Type])
    {
        case Type::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case Type::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            throw InvalidReportID();
    }
}

void Report::setType(Report::Type type)
{
    switch(type)
    {
        case Type::Short:
            _data.resize(HeaderLength + ShortParamLength);
            break;
        case Type::Long:
            _data.resize(HeaderLength + LongParamLength);
            break;
        default:
            throw InvalidReportID();
    }

    _data[Offset::Type] = type;
}

void Report::setParams(const std::vector<uint8_t>& _params)
{
    assert(_params.size() <= _data.size()-HeaderLength);

    for(std::size_t i = 0; i < _params.size(); i++)
        _data[Offset::Parameters + i] = _params[i];
}

bool Report::isError10(Report::hidpp10_error *error)
{
    assert(error != nullptr);

    if(_data[Offset::Type] != Type::Short ||
        _data[Offset::SubID] != hidpp10::ErrorID)
        return false;

    error->sub_id = _data[3];
    error->address = _data[4];
    error->error_code = _data[5];

    return true;
}

bool Report::isError20(Report::hidpp20_error* error)
{
    if(_data[Offset::Type] != Type::Long ||
        _data[Offset::Feature] != hidpp20::ErrorID)
        return false;

    error->feature_index= _data[3];
    error->function = (_data[4] >> 4) & 0x0f;
    error->software_id = _data[4] & 0x0f;
    error->error_code = _data[5];

    return true;
}