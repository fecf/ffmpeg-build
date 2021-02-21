using System;

namespace FfmpegBink.Interop
{
    public readonly ref struct VideoFrame
    {
        public double Time { get; }

        public int Width { get; }

        public int Height { get; }

        /// <summary>
        /// Pixel data in RGB32 format
        /// </summary>
        public ReadOnlySpan<byte> PixelData { get; }

        /// <summary>
        /// Stride in byte for PixelData.
        /// </summary>
        public int Stride { get; }

        public VideoFrame(double time, int width, int height, ReadOnlySpan<byte> pixelData, int stride)
        {
            Time = time;
            Width = width;
            Height = height;
            PixelData = pixelData;
            Stride = stride;
        }
    }
}