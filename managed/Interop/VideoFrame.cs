using System;

namespace FfmpegBink.Interop
{
    public readonly ref struct VideoFrame
    {
        public double Time { get; }

        public int Width { get; }

        public int Height { get; }

        public ReadOnlySpan<byte> YPlane { get; }
        public ReadOnlySpan<byte> UPlane { get; }
        public ReadOnlySpan<byte> VPlane { get; }

        public int YStride { get; }
        public int UStride { get; }
        public int VStride { get; }

        public VideoFrame(double time, int width, int height, ReadOnlySpan<byte> yPlane, ReadOnlySpan<byte> uPlane,
            ReadOnlySpan<byte> vPlane, int yStride, int uStride, int vStride)
        {
            Time = time;
            Width = width;
            Height = height;
            YPlane = yPlane;
            UPlane = uPlane;
            VPlane = vPlane;
            YStride = yStride;
            UStride = uStride;
            VStride = vStride;
        }
    }
}