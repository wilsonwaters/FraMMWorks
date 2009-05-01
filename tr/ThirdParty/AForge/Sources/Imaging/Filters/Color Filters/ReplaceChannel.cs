// AForge Image Processing Library
// AForge.NET framework
//
// Copyright � Andrew Kirillov, 2005-2007
// andrew.kirillov@gmail.com
//

namespace AForge.Imaging.Filters
{
    using System;
    using System.Drawing;
    using System.Drawing.Imaging;

    /// <summary>
    /// Replace RGB channel of color imgae.
    /// </summary>
    /// 
    /// <remarks>Replaces specified RGB channel of color image with
    /// specified grayscale image.</remarks>
    /// 
    public class ReplaceChannel : FilterColorToColorPartial
    {
        private short channel = RGB.R;
        private Bitmap channelImage;

        /// <summary>
        /// RGB channel to replace.
        /// </summary>
        /// 
        public short Channel
        {
            get { return channel; }
            set
            {
                if (
                    ( value != RGB.R ) &&
                    ( value != RGB.G ) &&
                    ( value != RGB.B )
                    )
                {
                    throw new ArgumentException( "Incorrect channel was specified" );
                }
                channel = value;
            }
        }

        /// <summary>
        /// Grayscale image to use for channel replacement.
        /// </summary>
        /// 
        public Bitmap ChannelImage
        {
            get { return channelImage; }
            set
            {
                // check for valid format
                if ( ( value != null ) && ( value.PixelFormat != PixelFormat.Format8bppIndexed ) )
                    throw new ArgumentException( "Channel image should be 8bpp indexed image (grayscale)" );

                channelImage = value;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ReplaceChannel"/> class.
        /// </summary>
        /// 
        /// <param name="channel">RGB channel to replace.</param>
        /// <param name="channelImage">Channel image to use for replacement.</param>
        /// 
        public ReplaceChannel( short channel, Bitmap channelImage )
        {
            this.Channel = channel;
            this.ChannelImage = channelImage;
        }

        /// <summary>
        /// Process the filter on the specified image.
        /// </summary>
        /// 
        /// <param name="imageData">Image data.</param>
        /// <param name="rect">Image rectangle for processing by the filter.</param>
        /// 
        protected override unsafe void ProcessFilter( BitmapData imageData, Rectangle rect )
        {
            int width   = imageData.Width;
            int height  = imageData.Height;
            int startX  = rect.Left;
            int startY  = rect.Top;
            int stopX   = startX + rect.Width;
            int stopY   = startY + rect.Height;
            int offset  = imageData.Stride - rect.Width * 3;

            // check channel image
            if ( channelImage == null )
                throw new ArgumentException( "Channel image was not specified" );

            // check channel's pixel format
            if ( channelImage.PixelFormat != PixelFormat.Format8bppIndexed )
                throw new ArgumentException( "Channel image should be 8bpp indexed image (grayscale)" );

            // check channel's image dimension
            if ( ( width != channelImage.Width ) || ( height != channelImage.Height ) )
                throw new ArgumentException( "Channel image size does not match source image size" );

            // lock channel image
            BitmapData chData = channelImage.LockBits(
                new Rectangle( 0, 0, width, height ),
                ImageLockMode.ReadOnly, PixelFormat.Format8bppIndexed );

            int offsetCh = chData.Stride - rect.Width;

            // do the job
            byte* dst = (byte*) imageData.Scan0.ToPointer( );
            byte* ch = (byte*) chData.Scan0.ToPointer( );

            // allign pointers to the first pixel to process
            dst += ( startY * imageData.Stride + startX * 3 );
            ch  += ( startY + chData.Stride + startX );

            // for each line
            for ( int y = startY; y < stopY; y++ )
            {
                // for each pixel
                for ( int x = startX; x < stopX; x++, dst += 3, ch++ )
                {
                    dst[channel] = *ch;
                }
                dst += offset;
                ch += offsetCh;
            }
            // unlock
            channelImage.UnlockBits( chData );
        }
    }
}
