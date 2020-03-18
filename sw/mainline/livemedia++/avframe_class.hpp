#pragma once


/*
 	 interface class for AVFrame of FFMpeg
 */
class avframe_class
{
public:
	avframe_class() :
		_frame(av_frame_alloc())
	{

	}
	avframe_class(const  AVFrame &frm) :
		_frame(av_frame_alloc())
	{
		av_frame_ref(_frame, &const_cast<AVFrame &>(frm));
	}
	avframe_class(const  avframe_class &frm) :
	_frame(av_frame_alloc())
	{
		av_frame_ref(_frame,
				const_cast<avframe_class &>(frm)._frame);
	}
	virtual ~avframe_class()
	{
		if(_frame)av_frame_free(&_frame);
		_frame = nullptr;
	}
	avframe_class &operator = (const  avframe_class &frm)
	{
		if(_frame)
			av_frame_free(&_frame);

		_frame = av_frame_alloc();
		av_frame_ref(_frame,
				const_cast<avframe_class &>(frm)._frame);
		return *this;
	}
	avframe_class &operator = (const  AVFrame &frm)
	{
		if(_frame)
			av_frame_free(&_frame);

		_frame = av_frame_alloc();
		av_frame_ref(_frame, &const_cast<AVFrame &>(frm));
		return *this;
	}
	/*
	 	 	 wanning! return direct pointer
	 */
	AVFrame *raw()
	{
		return _frame;
	}
	void unref()
	{
		av_frame_unref(_frame);
	}

	/*
	 	 move this->frame to return destination
	 */
	avframe_class move()
	{
		avframe_class dump;
		av_frame_move_ref(dump._frame, _frame);
		return dump;
	}


protected:
	AVFrame *_frame;
};



template <typename _raw_media_type>
class avframe_class_type : public avframe_class
{
private:
	virtual void field_attr_value(_raw_media_type &t) = 0;
	virtual void field_data_value(_raw_media_type &t)
	{
		int l = len();
		uint8_t *ptr = data_alloc(t.allocator());
		t.replace(ptr, l);
	}
protected:
	avframe_class_type() : avframe_class(){}
	avframe_class_type(const  AVFrame &frm) :
		avframe_class(frm){}
	avframe_class_type(const  avframe_class_type &_class) :
		avframe_class(dynamic_cast<const avframe_class &>(_class)){}



public:
	virtual int len() = 0;
	virtual void data_copy(uint8_t *ptr, int length) = 0;
	virtual uint8_t *data_alloc(const base_allocator &allocator) = 0;
	avframe_class_type &operator =
			(const  avframe_class_type &_class)
	{
		avframe_class::operator =
				(dynamic_cast<const avframe_class &>(_class));
		return *this;
	}
	avframe_class &operator =
			(const  AVFrame &frm)
	{
		avframe_class::operator =(frm);
		return *this;
	}
	_raw_media_type &operator >>
	(_raw_media_type &t)
	{
		field_data_value(t);
		field_attr_value(t);
		return t;
	}
	enum AVMediaType type() const
	{
		if(_frame->width > 0 &&
				_frame->height > 0 &&
				_frame->format != AV_PIX_FMT_NONE)
		{
			return AVMEDIA_TYPE_VIDEO;
		}
		else if(_frame->channels > 0 &&
				_frame->sample_rate > 0 &&
				_frame->format != AV_SAMPLE_FMT_NONE)
		{
			return AVMEDIA_TYPE_AUDIO;
		}
		return AVMEDIA_TYPE_UNKNOWN;
	}


};
