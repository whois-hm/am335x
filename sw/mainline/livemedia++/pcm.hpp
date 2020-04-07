#pragma once
/*
 	 	 audio raw data class
 */
class pcm :
		public raw_media_data
{
private:
	std::tuple <int,/*channel*/
	int,/*samplingrate*/
	int,/*samplesize*/
	enum AVSampleFormat/*sample format*/
	> _val;
public:
	/*
	 	 because reqeust size always > 0
	 */
	pcm() : raw_media_data(AVMEDIA_TYPE_AUDIO),
	_val(std::make_tuple(0, 0, 0, AV_SAMPLE_FMT_NONE)) { }
	pcm(const pcm &rhs) :
		raw_media_data (dynamic_cast<const raw_media_data &>(rhs)),
		_val(rhs._val) { }
	pcm(pcm &&rhs) :
		raw_media_data (std::move(rhs)),
		_val(rhs._val) { }
	virtual ~pcm(){}
	pcm &operator =
			(const pcm &rhs)
	{
		raw_media_data::operator =
				(dynamic_cast<const raw_media_data &>(rhs));
		_val = rhs._val;
		return *this;
	}
	pcm &operator =
			(pcm &&rhs)
	{
		raw_media_data::operator =
				(std::move(rhs));
		_val = rhs._val;
		return *this;
	}

	void set(int channel,
			int samplingrate,
			int samplesize,
			enum AVSampleFormat format)
	{
		_val = std::make_tuple(channel,
				samplingrate,
				samplesize,
				format);
	}
	void set(std::tuple<int,
			int,
			int,
			enum AVSampleFormat> &&val)
	{
		_val = val;
	}

	int channel() const
	{ return std::get<0>(_val); }
	int samplingrate() const
	{ return std::get<1>(_val); }
	int samplesize() const
	{ return std::get<2>(_val); }
	enum AVSampleFormat format() const
	{ return std::get<3>(_val); }
	virtual bool can_take()
	{
		/*
		 	 responsor filled field
		 */
		return std::get<0>(_val) > 0 &&
				std::get<1>(_val) > 0 &&
				std::get<2>(_val) > 0 &&
				std::get<3>(_val) != AV_SAMPLE_FMT_NONE &&
				raw_media_data::can_take();
	}
	pcm *clone()
	{
		return new pcm(*this);
	}
};
