// AForge Image Processing Library
// AForge.NET framework
//
// Copyright � Andrew Kirillov, 2005-2007
// andrew.kirillov@gmail.com
//
// Original idea found in Paint.NET project
// http://www.eecs.wsu.edu/paint.net/
//
namespace AForge.Imaging.Filters
{
    using System;
    using System.Drawing;
    using System.Drawing.Imaging;

    /// <summary>
    /// Oil Painting filter.
    /// </summary>
    /// 
    public class OilPainting : FilterAnyToAnyUsingCopyPartial
    {
        private int brushSize = 5;

        /// <summary>
        /// Brush size.
        /// </summary>
        /// 
        /// <remarks>Default value is 5. Minimum value is 3. Maximum value 21.</remarks>
        /// 
        public int BrushSize
        {
            get { return brushSize; }
            set { brushSize = Math.Max( 3, Math.Min( 21, value | 1 ) ); }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="OilPainting"/> class.
        /// </summary>
        public OilPainting( ) { }

        /// <summary>
        /// Initializes a new instance of the <see cref="OilPainting"/> class.
        /// </summary>
        /// 
        /// <param name="brushSize">Brush size.</param>
        /// 
        public OilPainting( int brushSize )
        {
            BrushSize = brushSize;
        }

        /// <summary>
        /// Process the filter on the specified image.
        /// </summary>
        /// 
        /// <param name="sourceData">Pointer to source image data (first scan line).</param>
        /// <param name="destinationData">Destination image data.</param>
        /// <param name="rect">Image rectangle for processing by the filter.</param>
        /// 
        protected override unsafe void ProcessFilter( IntPtr sourceData, BitmapData destinationData, Rectangle rect )
        {
            int pixelSize = ( destinationData.PixelFormat == PixelFormat.Format8bppIndexed ) ? 1 : 3;

            // processing start and stop X,Y positions
            int startX = rect.Left;
            int startY  = rect.Top;
            int stopX   = startX + rect.Width;
            int stopY   = startY + rect.Height;

            int stride = destinationData.Stride;
            int offset = stride - rect.Width * pixelSize;

            // loop and array indexes
            int i, j, t;
            // brush radius
            int radius = brushSize >> 1;

            // intensity values
            byte intensity, maxIntensity;
            int[] intensities = new int[256];

            byte* src = (byte*) sourceData.ToPointer( );
            byte* dst = (byte*) destinationData.Scan0.ToPointer( );
            byte* p;

            // allign pointers to the first pixel to process
            src += ( startY * stride + startX * pixelSize );
            dst += ( startY * stride + startX * pixelSize );

            if ( destinationData.PixelFormat == PixelFormat.Format8bppIndexed )
            {
                // Grayscale image

                // for each line
                for ( int y = startY; y < stopY; y++ )
                {
                    // for each pixel
                    for ( int x = startX; x < stopX; x++, src++, dst++ )
                    {
                        // clear arrays
                        Array.Clear( intensities, 0, 256 );

                        // for each kernel row
                        for ( i = -radius; i <= radius; i++ )
                        {
                            t = y + i;

                            // skip row
                            if ( t < startY )
                                continue;
                            // break
                            if ( t >= stopY )
                                break;

                            // for each kernel column
                            for ( j = -radius; j <= radius; j++ )
                            {
                                t = x + j;

                                // skip column
                                if ( t < startX )
                                    continue;

                                if ( t < stopX )
                                {
                                    intensity = src[i * stride + j];
                                    intensities[intensity]++;
                                }
                            }
                        }

                        // get most frequent intesity
                        maxIntensity = 0;
                        j = 0;

                        for ( i = 0; i < 256; i++ )
                        {
                            if ( intensities[i] > j )
                            {
                                maxIntensity = (byte) i;
                                j = intensities[i];
                            }
                        }

                        // set destination pixel
                        *dst = maxIntensity;
                    }
                    src += offset;
                    dst += offset;
                }
            }
            else
            {
                // RGB image
                int[] red = new int[256];
                int[] green = new int[256];
                int[] blue = new int[256];

                // for each line
                for ( int y = startY; y < stopY; y++ )
                {
                    // for each pixel
                    for ( int x = startX; x < stopX; x++, src += 3, dst += 3 )
                    {
                        // clear arrays
                        Array.Clear( intensities, 0, 256 );
                        Array.Clear( red, 0, 256 );
                        Array.Clear( green, 0, 256 );
                        Array.Clear( blue, 0, 256 );

                        // for each kernel row
                        for ( i = -radius; i <= radius; i++ )
                        {
                            t = y + i;

                            // skip row
                            if ( t < startY )
                                continue;
                            // break
                            if ( t >= stopY )
                                break;

                            // for each kernel column
                            for ( j = -radius; j <= radius; j++ )
                            {
                                t = x + j;

                                // skip column
                                if ( t < startX )
                                    continue;

                                if ( t < stopX )
                                {
                                    p = &src[i * stride + j * 3];

                                    // grayscale value using BT709
                                    intensity = (byte) ( 0.2125 * p[RGB.R] + 0.7154 * p[RGB.G] + 0.0721 * p[RGB.B] );

                                    //
                                    intensities[intensity]++;
                                    // red
                                    red[intensity] += p[RGB.R];
                                    // green
                                    green[intensity] += p[RGB.G];
                                    // blue
                                    blue[intensity] += p[RGB.B];
                                }
                            }
                        }

                        // get most frequent intesity
                        maxIntensity = 0;
                        j = 0;

                        for ( i = 0; i < 256; i++ )
                        {
                            if ( intensities[i] > j )
                            {
                                maxIntensity = (byte) i;
                                j = intensities[i];
                            }
                        }

                        // set destination pixel
                        dst[RGB.R] = (byte) ( red[maxIntensity] / intensities[maxIntensity] );
                        dst[RGB.G] = (byte) ( green[maxIntensity] / intensities[maxIntensity] );
                        dst[RGB.B] = (byte) ( blue[maxIntensity] / intensities[maxIntensity] );
                    }
                    src += offset;
                    dst += offset;
                }
            }
        }
    }
}
