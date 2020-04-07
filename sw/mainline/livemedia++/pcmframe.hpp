/*
 	 audio frame
 */

class pcmframe :
		public avframe_class_type
		<pcm>
{
private:
	void throw_if_not_audio()
	{
		DECLARE_THROW(type() != AVMEDIA_TYPE_AUDIO,
			"pcm frame type no match");
	}
	virtual void field_attr_value(pcm &t)
	{
		t.set(std::tuple<int,
			int,
			int,
			enum AVSampleFormat>(raw()->channels,
					raw()->sample_rate,
					0,/*TODO calc samplesize*/
					(enum AVSampleFormat)raw()->format));
	}
public:
	pcmframe() = delete;
	pcmframe(const  AVFrame &frm) :
		avframe_class_type(frm)
	{
		throw_if_not_audio();
	}
	pcmframe(const pcmframe &_class) :
		avframe_class_type(dynamic_cast<const avframe_class_type &>(_class))
	{
		throw_if_not_audio();
	}
	virtual ~pcmframe()
	{

	}
	pcmframe &operator = (const  AVFrame &frm)
	{
		avframe_class::operator =(frm);
		throw_if_not_audio();
		return *this;
	}
	pcmframe &operator = (const pcmframe &_class)
	{
		avframe_class_type::operator =
				(dynamic_cast<const avframe_class_type &>(_class));
		return *this;
	}
	virtual int len()
	{
		/*
		 	 For audio, only linesize[0] may be set. For planar audio, each channel plane must be the same size.
		 	 perhaps return value == first argument * raw()->channels
		 */
		/*
		 	 	      * For planar audio, each channel has a separate data pointer, and
		 	 	      * linesize[0] contains the size of each channel buffer.
		 	 	      * For packed audio, there is just one data pointer, and linesize[0]
		 	 	      * contains the total size of the buffer for all channels.
		 */
			return av_samples_get_buffer_size(NULL,
						 raw()->channels,
						 raw()->nb_samples,
				         (enum AVSampleFormat)raw()->format,
						 1);
	}
	virtual void data_copy(uint8_t *ptr, int length)
	{
		DECLARE_THROW(!ptr || length < len(), "pixelframe invalid parameter");
		/*
		 	 	 we operation pcm 1array style, then
		 	 	 no use 'av_samples_copy' function
		 */
		memcpy(ptr, raw()->extended_data[0], length);

	}
	virtual uint8_t *data_alloc(const base_allocator &allocator)
	{
		uint8_t *ptr = nullptr;
		int length = len();

		if(length > 0)
		{
			ptr= (uint8_t *)allocator(length);
			memcpy(ptr, raw()->extended_data[0], length);
		}
		return ptr;
	}
};


/*
 	 	 using presentation time stamp
 */

struct pcmframe_pts : public pcmframe
{
	double _pts;
	pcmframe_pts(const pcmframe &f) : pcmframe(f), _pts(0.0){}
	void operator()(const AVRational &rational)
	{
		_pts = test_pts(rational);
	}
	double test_pts(const AVRational &rational)
	{
		/*
		 	 audio need pts outside
		 */
		double test_val = 0.0;
		if(raw()->pkt_dts != AV_NOPTS_VALUE)
		{
			test_val = av_q2d(rational) * raw()->pkt_pts;
		}
		return test_val;
	}

};

