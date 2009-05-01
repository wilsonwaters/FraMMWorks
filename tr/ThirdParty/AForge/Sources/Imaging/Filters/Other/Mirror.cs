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
    /// Mirroring filter.
    /// </summary>
    /// 
    /// <remarks><para>The filter mirrors image around X and/or Y axis (horizontal and vertical
    /// mirroring).</para>
    /// <para>Sample usage:</para>
    /// <code>
    /// // create filter
    /// Mirror filter = new Mirror( false, true );
    /// // apply the filter
    /// filter.ApplyInPlace( image );
    /// </code>
    /// <para><b>Initial image:</b></para>
    /// <img src="sample1.jpg" width="480" height="361" />
    /// <para><b>Result image:</b></para>
    /// <img src="mirror.jpg" width="480" height="361" />
    /// </remarks>
    /// 
    public class Mirror : FilterAnyToAnyPartial
    {
        private bool mirrorX = false;
        private bool mirrorY = false;

        /// <summary>
        /// Specifies if mirroring should be done for X axis (horizontal mirroring).
        /// </summary>
        /// 
        public bool MirrorX
        {
            get { return mirrorX; }
            set { mirrorX = value; }
        }

        /// <summary>
        /// Specifies if mirroring should be done for Y axis (vertical mirroring).
        /// </summary>
        /// 
        public bool MirrorY
        {
            get { return mirrorY; }
            set { mirrorY = value; }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Mirror"/> class.
        /// </summary>
        /// 
        /// <param name="mirrorX">Specifies if mirroring should be done for X axis.</param>
        /// <param name="mirrorY">Specifies if mirroring should be done for Y axis</param>
        /// 
        public Mirror( bool mirrorX, bool mirrorY )
        {
            this.mirrorX = mirrorX;
            this.MirrorY = mirrorY;
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
            int pixelSize = ( imageData.PixelFormat == PixelFormat.Format8bppIndexed ) ? 1 : 3;

            int width   = rect.Width;
            int height  = rect.Height;

            // processing start and stop X,Y positions
            int startY  = rect.Top;
            int stopY   = startY + height;
            int startX  = rect.Left;
            int stopX   = startX + width;
            int startXInBytes = startX * pixelSize;
            int stopXInBytes  = stopX * pixelSize;

            int stride = imageData.Stride;

            // perform Y mirroring
            if ( mirrorY )
            {
                // first pointer - points to the first pixel in line
                byte* ptr1 = (byte*) imageData.Scan0.ToPointer( );
                ptr1 += ( startY * stride + startX * pixelSize );
                // second pointer - points to the last pixel in line
                byte* ptr2 = (byte*) imageData.Scan0.ToPointer( );
                ptr2 += ( startY * stride + ( stopX - 1 ) * pixelSize );

                // offsets
                int offset1 = stride - ( width >> 1 ) * pixelSize;
                int offset2 = stride + ( width >> 1 ) * pixelSize;

                // temporary value for swapping
                byte v;

                if ( imageData.PixelFormat == PixelFormat.Format8bppIndexed )
                {
                    // grayscale mirroring

                    // for each line
                    for ( int y = startY; y < stopY; y++ )
                    {
                        // for each pixel
                        for ( int x = startX, halfStopX = startX + ( width >> 1 ); x < halfStopX; x++, ptr1++, ptr2-- )
                        {
                            // swap values
                            v = *ptr1;
                            *ptr1 = *ptr2;
                            *ptr2 = v;
                        }
                        ptr1 += offset1;
                        ptr2 += offset2;
                    }
                }
                else
                {
                    // color mirroring

                    // for each line
                    for ( int y = startY; y < stopY; y++ )
                    {
                        // for each pixel
                        for ( int x = startX, halfStopX = startX + ( width >> 1 ); x < halfStopX; x++, ptr1 += 3, ptr2 -= 3 )
                        {
                            // swap Red
                            v = ptr1[RGB.R];
                            ptr1[RGB.R] = ptr2[RGB.R];
                            ptr2[RGB.R] = v;

                            // swap Green
                            v = ptr1[RGB.G];
                            ptr1[RGB.G] = ptr2[RGB.G];
                            ptr2[RGB.G] = v;

                            // swap Blue
                            v = ptr1[RGB.B];
                            ptr1[RGB.B] = ptr2[RGB.B];
                            ptr2[RGB.B] = v;
                        }
                        ptr1 += offset1;
                        ptr2 += offset2;
                    }
                }
            }

            // perform X mirroring
            if ( mirrorX )
            {
                int offset = stride - rect.Width * pixelSize;

                // first pointer - points to the first line
                byte* ptr1 = (byte*) imageData.Scan0.ToPointer( );
                ptr1 += ( startY * stride + startX * pixelSize );
                // second pointer - points to the last line
                byte* ptr2 = (byte*) imageData.Scan0.ToPointer( );
                ptr2 += ( ( stopY - 1 ) * stride + startX * pixelSize );

                // temporary value for swapping
                byte v;

                // for each line
                for ( int y = startY, halfStopY = startY + ( height >> 1 ); y < halfStopY; y++ )
                {
                    // for each pixel
                    for ( int x = startXInBytes; x < stopXInBytes; x++, ptr1++, ptr2++ )
                    {
                        // swap values
                        v = *ptr1;
                        *ptr1 = *ptr2;
                        *ptr2 = v;
                    }
                    ptr1 += offset;
                    ptr2 += offset - stride - stride;
                }
            }
        }
    }
}
