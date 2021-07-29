#ifndef __OSCILLOSCOPE_HPP
#define __OSCILLOSCOPE_HPP

#include <autogen/interfaces.hpp>

class Oscilloscope : public ODriveIntf::OscilloscopeIntf {
public:
    Oscilloscope();
    //Oscilloscope(float* trigger_src, float trigger_threshold, float** data_src);

    float get_val(uint32_t index) override;

    void update();

    //const float* trigger_src_;
    //const float trigger_threshold_;
    //float* const * data_src_;
	uint32_t loop_counter_ = 0;

	const uint32_t size_;

    bool ready_ = false;
    bool capturing_ = false;

	bool force_trigger_ = false;
    int64_t trigger_counter_ = 0;
};

#endif // __OSCILLOSCOPE_HPP
