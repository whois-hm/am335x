#pragma once


/*
	input source from other threads data
*/
template <typename T>/*
							template T 
							only  live5scheduler<T> use for 'register trigger'
						*/

class live5extsource : public FramedSource
{
private:
	live5scheduler<T> &_scheduler;
	/*
		trigger event id from another thread
	*/
	unsigned _trigger_id;
	unsigned _buffersize;
	unsigned char *_buffer;
	unsigned _bufferindx;
	bool _putis_lock;
	/*
		locking method for put 
		lock the put thread if data was full 
	*/
	semaphore _lock_put;
	/*
		input : your thread 
		output : live555 shceduled thread
	*/
	std::mutex _lock;

	virtual void doGetNextFrame()
	{
		/*	stop.
			we offered next frame by event trigger 'dogetnextframe_from_bufer'
		*/
	}
	void dogetnextframe_from_buffer()
	{
		return get();
	}
	void get()
	{
		_lock.lock();

		/*
			calculate read avail
		*/
		unsigned read_avail_size = _bufferindx;
		unsigned read_reqeust_size = fMaxSize;
		unsigned remain_size = 0;
		fFrameSize = read_avail_size >= read_reqeust_size ? read_reqeust_size : read_avail_size;
		fNumTruncatedBytes = read_reqeust_size >= fFrameSize ? 0 : read_reqeust_size - fFrameSize;
		remain_size = read_avail_size - fFrameSize;

		/*
			copy data to source or sink.
		*/
		memcpy(fTo, _buffer, fFrameSize);
		if(remain_size)
			memcpy(_buffer, _buffer + fFrameSize, remain_size);
		
		_bufferindx = remain_size;
		/*
			wake up 'put' thread if sleep
		*/
		if(_putis_lock)
		{
			SEMA_unlock(&_lock_put);
		}
		_lock.unlock();


		/*
			pts write
		*/
		gettimeofday(&fPresentationTime, nullptr);

		/*
			deliver to aftergetting function ptr
		*/
		FramedSource::afterGetting(this);		
	}
	public:
		virtual ~live5extsource()
		{
			_lock.lock();
			/*
				wake up 'put' thread if sleep
			*/
			if(_putis_lock)
			{
				SEMA_unlock(&_lock_put);
			}
			_lock.unlock();
			__base__free__(_buffer);
		}
		live5extsource(live5scheduler<T> &scheduler, 
			unsigned trigger_id,
			UsageEnvironment &env,
			unsigned buffersize) :
			FramedSource(env), 
				_scheduler(scheduler),
				_trigger_id(trigger_id),
				_buffersize(buffersize),
				_buffer((unsigned char *)__base__malloc__(_buffersize)),
				_bufferindx(0),
				_putis_lock(false)
			{
				throw_if ti;				
				ti(_buffersize < 0, "live5extsource can't create buffersize zero");
				ti(!_buffer, "live5extsource can't create buffer is null pointer");
				ti(SEMA_open(&_lock_put, 0,1) < 0, "live5extsource can't create semaphore has error");

				_scheduler.register_trigger(_trigger_id, 
					[]()->void{
						/*
							deliver data to request sink or source
						*/
						dogetnextframe_from_buffer();
					}
				);
			}

		void put(unsigned char *ptr, unsigned nptr)
		{
			/*
				invalid parameter
			*/
			unsigned usrindx = 0;
			while(nptr > 0 && 
				ptr)
			{
				_lock.lock();				
				unsigned remain_size = _buffersize - _bufferindx;
				_putis_lock = remain_size <= 0; 				
				if(!_putis_lock)
				{	
					/*
						copy the buffer if available writing size
					*/
					unsigned write_size = remain_size >= nptr ? nptr : remain_size; 				
					memcpy(_buffer + _bufferindx, ptr + usrindx, write_size);
					usrindx += write_size;
					nptr -= write_size;
					_bufferindx += write_size;
				}
				_lock.unlock();
				/*
					thread sleep untill write enable
				*/
				if(_putis_lock)
				{
					SEMA_lock(&_lock_put, INFINITE);
				}
			}	

			/*
				if success writing in the buffer,
				signal sending to scheudler for calling 'get()'
			*/
			if(usrindx)
				_scheduler.trigger(_trigger_id);
		}
};
