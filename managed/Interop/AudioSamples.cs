using System;

namespace FfmpegBink.Interop
{
    public readonly ref struct AudioSamples
    {
        public ReadOnlySpan<float> Plane1 { get; }
        public ReadOnlySpan<float> Plane2 { get; }
        public int ChannelCount { get; }
        public bool IsInterleaved { get; }

        public AudioSamples(ReadOnlySpan<float> plane1, ReadOnlySpan<float> plane2, int channelCount, bool isInterleaved)
        {
            Plane1 = plane1;
            Plane2 = plane2;
            ChannelCount = channelCount;
            IsInterleaved = isInterleaved;
        }
    }
}