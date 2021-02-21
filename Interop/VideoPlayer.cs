using System;
using System.Runtime.InteropServices;

namespace FfmpegBink.Interop
{
    public delegate void VideoFrameCallback(in VideoFrame frame);

    public delegate void AudioSamplesCallback(in AudioSamples samples);

    public class VideoPlayer : IDisposable
    {
        private const int PlaneY = 0;
        private const int PlaneU = 1;
        private const int PlaneV = 2;

        private const string DllPath = "ffmpeg-bink.dll";

        private IntPtr _handle;

        private unsafe delegate void NativeVideoFrameDelegate(double time, byte** planes, int* strides);

        private unsafe delegate void NativeAudioSamplesDelegate(float** planes, int sampleCount,
            [MarshalAs(UnmanagedType.Bool)]
            bool interleaved);

        private delegate void NativeStopDelegate();

        private readonly NativeVideoFrameDelegate _onNativeVideoFrame;
        private readonly NativeAudioSamplesDelegate _onNativeAudioSamples;
        private readonly NativeStopDelegate _onNativeStopDelegate;

        public event VideoFrameCallback OnVideoFrame;

        public event AudioSamplesCallback OnAudioSamples;

        public event Action OnStop;

        public VideoPlayer()
        {
            _handle = BinkPlayer_Create();
            if (_handle == IntPtr.Zero)
            {
                throw new InvalidOperationException("Failed to create BinkPlayer.");
            }

            unsafe
            {
                _onNativeVideoFrame = OnNativeVideoFrame;
                _onNativeAudioSamples = OnNativeAudioSamples;
                _onNativeStopDelegate = OnNativeStop;
            }
        }

        public bool Open(string path) => BinkPlayer_Open(GetHandleSafe(), path);

        public void Play() => BinkPlayer_Play(GetHandleSafe(), _onNativeVideoFrame, _onNativeAudioSamples,
            _onNativeStopDelegate);

        public void Stop() => BinkPlayer_Stop(GetHandleSafe());

        public bool AtEnd => BinkPlayer_AtEnd(GetHandleSafe());

        public string Error => BinkPlayer_Error(GetHandleSafe());

        public bool HasVideo => BinkPlayer_HasVideo(GetHandleSafe());

        public int VideoWidth => BinkPlayer_Width(GetHandleSafe());

        public int VideoHeight => BinkPlayer_Height(GetHandleSafe());

        public bool HasAudio => BinkPlayer_HasAudio(GetHandleSafe());

        public int AudioSampleRate => BinkPlayer_AudioSampleRate(GetHandleSafe());

        public int AudioChannels => BinkPlayer_AudioChannels(GetHandleSafe());

        private unsafe void OnNativeVideoFrame(double time, byte** planes, int* strides)
        {
            var width = VideoWidth;
            var height = VideoHeight;

            // Format is ALWAYS YUV420
            var yPlaneSize = height * strides[PlaneY];
            // U and V planes are sampled at half height, half width
            var uPlaneSize = height / 2 * strides[PlaneU];
            var vPlaneSize = height / 2 * strides[PlaneV];

            var frame = new VideoFrame(
                time,
                width,
                height,
                new ReadOnlySpan<byte>(planes[PlaneY], yPlaneSize),
                new ReadOnlySpan<byte>(planes[PlaneU], uPlaneSize),
                new ReadOnlySpan<byte>(planes[PlaneV], vPlaneSize),
                strides[PlaneY],
                strides[PlaneU],
                strides[PlaneV]
            );

            OnVideoFrame?.Invoke(in frame);
        }

        private unsafe void OnNativeAudioSamples(float** planes, int sampleCount, bool interleaved)
        {
            var channelCount = AudioChannels;
            var plane1 = channelCount >= 1
                ? new ReadOnlySpan<float>(planes[0], interleaved ? channelCount * sampleCount : sampleCount)
                : ReadOnlySpan<float>.Empty;
            var plane2 = channelCount >= 2 && !interleaved
                ? new ReadOnlySpan<float>(planes[1], sampleCount)
                : ReadOnlySpan<float>.Empty;

            var frame = new AudioSamples(
                plane1, plane2, channelCount, interleaved
            );

            OnAudioSamples?.Invoke(in frame);
        }

        private void OnNativeStop()
        {
            OnStop?.Invoke();
        }

        private IntPtr GetHandleSafe()
        {
            if (_handle == IntPtr.Zero)
            {
                throw new ObjectDisposedException("BinkPlayer");
            }

            return _handle;
        }

        [DllImport(DllPath)]
        private static extern IntPtr BinkPlayer_Create();

        [DllImport(DllPath)]
        private static extern void BinkPlayer_Free(IntPtr player);

        [DllImport(DllPath, CharSet = CharSet.Ansi)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool BinkPlayer_Open(IntPtr player, string path);

        [DllImport(DllPath)]
        private static extern void BinkPlayer_Play(IntPtr player, NativeVideoFrameDelegate onVideoFrame,
            NativeAudioSamplesDelegate onAudioSamples, NativeStopDelegate onStop);

        [DllImport(DllPath)]
        private static extern void BinkPlayer_Stop(IntPtr player);

        [DllImport(DllPath)]
        [return: MarshalAs(UnmanagedType.Bool)]
        [SuppressGCTransition]
        private static extern bool BinkPlayer_AtEnd(IntPtr player);

        [DllImport(DllPath)]
        [return: MarshalAs(UnmanagedType.LPStr)]
        private static extern string BinkPlayer_Error(IntPtr player);

        [DllImport(DllPath)]
        [return: MarshalAs(UnmanagedType.Bool)]
        [SuppressGCTransition]
        private static extern bool BinkPlayer_HasVideo(IntPtr player);

        [DllImport(DllPath)]
        [SuppressGCTransition]
        private static extern int BinkPlayer_Width(IntPtr player);

        [DllImport(DllPath)]
        [SuppressGCTransition]
        private static extern int BinkPlayer_Height(IntPtr player);

        [DllImport(DllPath)]
        [return: MarshalAs(UnmanagedType.Bool)]
        [SuppressGCTransition]
        private static extern bool BinkPlayer_HasAudio(IntPtr player);

        [DllImport(DllPath)]
        [SuppressGCTransition]
        private static extern int BinkPlayer_AudioSampleRate(IntPtr player);

        [DllImport(DllPath)]
        [SuppressGCTransition]
        private static extern int BinkPlayer_AudioChannels(IntPtr player);

        private void ReleaseUnmanagedResources()
        {
            if (_handle != IntPtr.Zero)
            {
                BinkPlayer_Free(_handle);
                _handle = IntPtr.Zero;
            }
        }

        public void Dispose()
        {
            ReleaseUnmanagedResources();
            GC.SuppressFinalize(this);
        }

        ~VideoPlayer()
        {
            ReleaseUnmanagedResources();
        }
    }
}