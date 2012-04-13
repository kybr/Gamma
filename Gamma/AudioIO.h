#ifndef GAMMA_AUDIOIO_H_INC
#define GAMMA_AUDIOIO_H_INC

/*	Gamma - Generic processing library
	See COPYRIGHT file for authors and license information

	File Description:
	Classes for performing audio input/output streaming
*/

#include <string>

namespace gam{

class AudioIOData;


/// Audio callback type
typedef void (* audioCallback)(AudioIOData& io);


/// Audio device abstraction
class AudioDevice{
public:

	/// Stream mode
	enum StreamMode{
		INPUT	= 1,	/**< Input stream */
		OUTPUT	= 2		/**< Output stream */
	};


	/// @param[in] deviceNum	Device enumeration number
	AudioDevice(int deviceNum);
	
	/// @param[in] nameKeyword	Keyword to search for in device name
	/// @param[in] stream		Whether to search for input and/or output devices
	AudioDevice(const std::string& nameKeyword, StreamMode stream = StreamMode(INPUT | OUTPUT));

	~AudioDevice();

	bool valid() const { return 0!=mImpl; }	///< Returns whether device is valid
	int id() const { return mID; }			///< Get device unique ID
	const char * name() const;				///< Get device name
	int channelsInMax() const;				///< Get maximum number of input channels supported
	int channelsOutMax() const;				///< Get maximum number of output channels supported
	double defaultSampleRate() const;		///< Get default sample rate
	
	bool hasInput() const;					///< Returns whether device has input
	bool hasOutput() const;					///< Returns whether device has output
	
	void print() const;						///< Prints info about specific i/o device to stdout

	static AudioDevice defaultInput();		///< Get system's default input device
	static AudioDevice defaultOutput();		///< Get system's default output device
	static int numDevices();				///< Returns number of audio i/o devices available
	static void printAll();					///< Prints info about all available i/o devices to stdout

private:
	void setImpl(int deviceNum);
	static void initDevices();
	int mID;
	const void * mImpl;
};

inline AudioDevice::StreamMode operator| (const AudioDevice::StreamMode& a, const AudioDevice::StreamMode& b){
	return static_cast<AudioDevice::StreamMode>(+a|+b);
}


/// Audio data to be sent to callback
class AudioIOData {
public:
	/// Constructor
	AudioIOData(void * user);

	virtual ~AudioIOData();


	/// Iterate frame counter, returning true while more frames
	bool operator()() const { return (++mFrame)<framesPerBuffer(); }
		
	/// Get current frame number
	int frame() const { return mFrame; }

	/// Get bus sample at current frame iteration on specified channel
	float& bus(int chan) const { return bus(chan, frame()); }

	/// Get bus sample at specified channel and frame
	float& bus(int chan, int frame) const;

	/// Get non-interleaved bus buffer on specified channel
	float * busBuffer(int chan=0) const { return &bus(chan,0); }

	/// Get input sample at current frame iteration on specified channel
	const float& in(int chan) const { return in (chan, frame()); }

	/// Get input sample at specified channel and frame
	const float& in (int chan, int frame) const;

	/// Get non-interleaved input buffer on specified channel
	const float * inBuffer(int chan=0) const { return &in(chan,0); }

	/// Get output sample at current frame iteration on specified channel
	float& out(int chan) const { return out(chan, frame()); }

	/// Get output sample at specified channel and frame
	float& out(int chan, int frame) const;

	/// Get non-interleaved output buffer on specified channel
	float * outBuffer(int chan=0) const { return &out(chan,0); }
	
	/// Add value to current output sample on specified channel
	void sum(float v, int chan) const { out(chan)+=v; }
	
	/// Add value to current output sample on specified channels
	void sum(float v, int ch1, int ch2) const { sum(v, ch1); sum(v,ch2); }
	
	/// Get sample from temporary buffer at specified frame
	float& temp(int frame) const;

	/// Get non-interleaved temporary buffer on specified channel
	float * tempBuffer() const { return &temp(0); }

	void * user() const{ return mUser; } ///< Get pointer to user data

	template<class UserDataType>
	UserDataType& user() const { return *(static_cast<UserDataType *>(mUser)); }

	int channelsIn () const;			///< Get effective number of input channels
	int channelsOut() const;			///< Get effective number of output channels
	int channelsBus() const;			///< Get number of allocated bus channels
	int channelsInDevice() const;		///< Get number of channels opened on input device
	int channelsOutDevice() const;		///< Get number of channels opened on output device
	int framesPerBuffer() const;		///< Get frames/buffer of audio I/O stream
	double framesPerSecond() const;		///< Get frames/second of audio I/O streams
	double fps() const { return framesPerSecond(); }
	double secondsPerBuffer() const;	///< Get seconds/buffer of audio I/O stream
	double time() const;				///< Get current stream time in seconds
	double time(int frame) const;		///< Get current stream time in seconds of frame


	void frame(int v){ mFrame=v-1; }	///< Set frame count for next iteration
	void zeroBus();						///< Zeros all the bus buffers
	void zeroOut();						///< Zeros all the internal output buffers

protected:
	class Impl; Impl * mImpl;
	void * mUser;					// User specified data
	mutable int mFrame;
	int mFramesPerBuffer;
	double mFramesPerSecond;
	float *mBufI, *mBufO, *mBufB;	// input, output, and aux buffers
	float * mBufT;					// temporary one channel buffer
	int mNumI, mNumO, mNumB;		// input, output, and aux channels
};



/// Audio input/output streaming.

/// This is a wrapper around the PortAudio v1.9 library.
/// 
class AudioIO : public AudioIOData {
public:

	/// Creates AudioIO using default I/O devices.
	///
	/// @param[in] framesPerBuf		Number of sample frames to process per callback
	/// @param[in] framesPerSec		Frame rate.  Unsupported values will use default rate of device.
	/// @param[in] callback			Audio processing callback
	/// @param[in] userData			Pointer to user data accessible within callback
	/// @param[in] outChans			Number of output channels to open
	/// @param[in] inChans			Number of input channels to open
	/// If the number of input or output channels is greater than the device
	/// supports, virtual buffers will be created.
	AudioIO(
		int framesPerBuf=64, double framesPerSec=44100.0,
		void (* callback)(AudioIOData &) = 0, void * userData = 0,
		int outChans = 2, int inChans = 0 );

	virtual ~AudioIO();

	using AudioIOData::channelsIn;
	using AudioIOData::channelsOut;
	using AudioIOData::framesPerBuffer;
	using AudioIOData::framesPerSecond;

	audioCallback callback;						///< User specified callback function.

	bool autoZeroOut() const { return mAutoZeroOut; }
	int channels(bool forOutput) const;
	bool clipOut() const { return mClipOut; }	///< Returns clipOut setting
	double cpu() const;							///< Returns current CPU usage of audio thread
	bool supportsFPS(double fps) const;			///< Return true if fps supported, otherwise false
	bool zeroNANs() const;						///< Returns whether to zero NANs in output buffer going to DAC
	
	void operator()();							///< Call callback manually
	bool open();								///< Opens audio device.
	bool close();								///< Closes audio device. Will stop active IO.
	bool start();								///< Starts the audio IO.  Will open audio device if necessary.
	bool stop();								///< Stops the audio IO.

	void autoZeroOut(bool v){ mAutoZeroOut=v; }

	/// Sets number of effective channels on input or output device depending on 'forOutput' flag.
	
	/// An effective channel is either a real device channel or virtual channel 
	/// depending on how many channels the device supports. Passing in -1 for
	/// the number of channels opens all available channels.
	void channels(int num, bool forOutput);

	void channelsIn(int n){channels(n,false);}	///< Set number of input channels
	void channelsOut(int n){channels(n,true);}	///< Set number of output channels
	void channelsBus(int num);					///< Set number of bus channels
	void clipOut(bool v){ mClipOut=v; }			///< Set whether to clip output between -1 and 1
	void deviceIn(const AudioDevice& v);		///< Set input device
	void deviceOut(const AudioDevice& v);		///< Set output device
	void framesPerSecond(double v);				///< Set number of frames per second
	void framesPerBuffer(int n);				///< Set number of frames per processing buffer
	void zeroNANs(bool v){ mZeroNANs=v; }		///< Set whether to zero NANs in output buffer going to DAC

	void print();								///< Prints info about current i/o devices to stdout.

	static const char * errorText(int errNum);		// Returns error string.

private:
	AudioDevice mInDevice, mOutDevice;

	bool mInResizeDeferred, mOutResizeDeferred;
	bool mZeroNANs;			// whether to zero NANs
	bool mClipOut;			// whether to clip output between -1 and 1
	bool mAutoZeroOut;		// whether to automatically zero output buffers each block

	void init();		// Initializes PortAudio and member variables.
	void deferBufferResize(bool forOutput);
	void reopen();		// reopen stream (restarts stream if needed)
	void resizeBuffer(bool forOutput);
};


//==============================================================================

inline float&       AudioIOData::bus(int c, int f) const { return mBufB[c*framesPerBuffer() + f]; }
inline const float& AudioIOData::in (int c, int f) const { return mBufI[c*framesPerBuffer() + f]; }
inline float&       AudioIOData::out(int c, int f) const { return mBufO[c*framesPerBuffer() + f]; }
inline float&       AudioIOData::temp(int f) const { return mBufT[f]; }

} // gam::

#endif
