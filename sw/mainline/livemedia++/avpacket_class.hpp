#pragma once
/*
 	 interface class AVPacket of FFMpeg
 */
class avpacket_class
{
public:
	avpacket_class( )
	{
		av_init_packet(&_pkt);
	}
	avpacket_class(   const AVPacket &pkt )
	{
		av_init_packet(&_pkt);

		av_packet_ref(&_pkt, &const_cast<AVPacket &>(pkt));
	}
	avpacket_class(   AVPacket &&pkt )
	{
		av_init_packet(&_pkt);

		av_packet_move_ref(&_pkt, &const_cast<AVPacket &>(pkt));
	}
	avpacket_class(   const avpacket_class &pkt )
	{
		av_init_packet(&_pkt);
		av_packet_ref(&_pkt, &const_cast<AVPacket &>(pkt._pkt));
	}
	avpacket_class(   avpacket_class &&pkt )
	{
		av_init_packet(&_pkt);
		av_packet_move_ref(&_pkt, &const_cast<AVPacket &>(pkt._pkt));
	}
	virtual ~avpacket_class()
	{
		unref();
	}
	avpacket_class &operator = (const  avpacket_class &pkt)
	{
		unref();
		av_packet_ref(&_pkt, &const_cast<AVPacket &>(pkt._pkt));
		return *this;
	}
	avpacket_class &operator = (avpacket_class &&pkt)
	{
		unref();
		av_packet_move_ref(&_pkt, &const_cast<AVPacket &>(pkt._pkt));
		return *this;
	}
	avpacket_class &operator = (const  AVPacket &pkt)
	{
		unref();
		av_packet_ref(&_pkt, &const_cast<AVPacket &>(pkt));
		return *this;
	}
	avpacket_class &operator = (AVPacket &&pkt)
	{
		unref();
		av_packet_move_ref(&_pkt, &const_cast<AVPacket &>(pkt));
		return *this;
	}
	AVPacket *raw()
	{
		return &_pkt;
	}
	void unref()
	{
		av_packet_unref(&_pkt);
	}
	avpacket_class move()
	{
		avpacket_class dump;
		av_packet_move_ref(&dump._pkt, &_pkt);

		return dump;
	}
	void fromdata(uint8_t *data, int size)
	{
		if(data && size > 0)
		{
			av_buffer_unref(&_pkt.buf);
			av_packet_from_data(&_pkt, data, size);
		}
	}
protected:
	AVPacket _pkt;

};
