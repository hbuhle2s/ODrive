
#include "oscilloscope.hpp"
#include "axis.hpp"

#if 1
// this is from: https://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
typedef unsigned short ushort;
typedef unsigned int uint;

inline uint as_uint(const float x) {
    union { float f; uint i; } val;
    val.f = x;
    return val.i;
}
inline float as_float(const uint x) {
    union { float f; uint i; } val;
    val.i = x;
    return val.f;
}

inline float half_to_float(ushort x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
    const uint e = (x&0x7C00)>>10; // exponent
    const uint m = (x&0x03FF)<<13; // mantissa
    const uint v = as_uint((float)m)>>23; // evil log2 bit hack to count leading zeros in denormalized format
    return as_float((x&0x8000)<<16 | (e!=0)*((e+112)<<23|m) | ((e==0)&(m!=0))*((v-37)<<23|((m<<(150-v))&0x007FE000))); // sign : normalized : denormalized
}
inline ushort float_to_half(float x) { // IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
    const uint b = as_uint(x)+0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
    const uint e = (b&0x7F800000)>>23; // exponent
    const uint m = b&0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
    return (b&0x80000000)>>16 | (e>112)*((((e-112)<<10)&0x7C00)|m>>13) | ((e<113)&(e>101))*((((0x007FF000+m)>>(125-e))+1)>>1) | (e>143)*0x7FFF; // sign : normalized : denormalized : saturate
}

using oscilloscope_type = ushort;
#else
using oscilloscope_type = float;
inline float half_to_float(float x) {
	return x;
}
inline float float_to_half(float x) {
	return x;
}
#endif

// if you use the oscilloscope feature you can bump up this value
//#define OSCILLOSCOPE_SIZE 4096
//#define OSCILLOSCOPE_SIZE 36000 // 0.5.1 version
#define OSCILLOSCOPE_SIZE 33000
#define OSCILLOSCOPE_NUM_AXES 2
oscilloscope_type oscilloscope[OSCILLOSCOPE_SIZE] = {0};
size_t oscilloscope_pos = 0;

//Oscilloscope::Oscilloscope(float* trigger_src, float trigger_threshold, float** data_src)
//        : trigger_src_(trigger_src), trigger_threshold_(trigger_threshold), data_src_(data_src),
//		  size_(OSCILLOSCOPE_SIZE*OSCILLOSCOPE_NUM_AXES)
//{}
Oscilloscope::Oscilloscope()
        : size_(OSCILLOSCOPE_SIZE)
{}

float Oscilloscope::get_val(uint32_t index) {
    return half_to_float(oscilloscope[index]);
}

void add_oscilloscope(float value) {
    if (oscilloscope_pos < OSCILLOSCOPE_SIZE) {
		oscilloscope[oscilloscope_pos] = float_to_half(value);
        oscilloscope_pos++;
    }
}

void Oscilloscope::update() {
    /*float trigger_data = trigger_src_ ? *trigger_src_ : 0.0f;
    float trigger_threshold = trigger_threshold_;
    float sample_data = data_src_ ? **data_src_ : 0.0f;

    if (trigger_data < trigger_threshold) {
        ready_ = true;
    }
    if (ready_ && trigger_data >= trigger_threshold) {
        capturing_ = true;
        ready_ = false;
    }
    if (capturing_) {
        if (pos_ < OSCILLOSCOPE_SIZE) {
            data_[pos_++] = sample_data;
        } else {
            pos_ = 0;
            capturing_ = false;
        }
    }*/

    // Edit these to suit your capture needs
    /*float trigger_data = ictrl.v_current_control_integral_d;
    float trigger_threshold = 0.5f;
    float sample_data = Ialpha;

    static bool ready = false;
    if (trigger_data < trigger_threshold) {
        ready = true;
    }
    if (ready && trigger_data >= trigger_threshold) {
        capturing = true;
        ready = false;
    }*/
	loop_counter_++;
    if (force_trigger_) {
        if (!capturing_) {
			capturing_ = true;
            trigger_counter_ = ((int64_t)1<<62) | (int64_t)loop_counter_;
        }
        force_trigger_ = false;
    }
    if (capturing_) {
        for (int axis_num = 0; axis_num < OSCILLOSCOPE_NUM_AXES; axis_num++) {
			Axis* axis_ = &axes[axis_num];
			add_oscilloscope(axis_->encoder_.pos_estimate_.any().value_or(0.0f));
			add_oscilloscope(axis_->motor_.current_control_.Idq_setpoint_->second);
			//add_oscilloscope(ictrl.Iq_measured);
			add_oscilloscope(axis_->controller_.pos_setpoint_);
        }

		if (oscilloscope_pos >= OSCILLOSCOPE_SIZE) {
			capturing_ = false;
            oscilloscope_pos = 0;
            trigger_counter_ = ((int64_t)1<<61) | (int64_t)(loop_counter_+1);
        }
    }
}
