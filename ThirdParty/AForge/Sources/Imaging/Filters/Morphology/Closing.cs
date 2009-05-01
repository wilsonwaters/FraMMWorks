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
	/// Closing operator from Mathematical Morphology.
	/// </summary>
	/// 
    /// <remarks><para>Closing morphology operator equals to <see cref="Dilatation">dilatation</see> followed
    /// by <see cref="Erosion">erosion</see>.</para>
    /// <para>Sample usage:</para>
    /// <code>
    /// // create filter
    /// Closing filter = new Closing( );
    /// // apply the filter
    /// filter.Apply( image );
    /// </code>
    /// </remarks>
	/// 
	public class Closing : IFilter, IInPlaceFilter, IInPlacePartialFilter
	{
        private Erosion     errosion = new Erosion( );
        private Dilatation  dilatation = new Dilatation( );

		/// <summary>
		/// Initializes a new instance of the <see cref="Closing"/> class.
		/// </summary>
        /// 
		public Closing( ) { }

		/// <summary>
		/// Initializes a new instance of the <see cref="Closing"/> class.
		/// </summary>
		/// 
		/// <param name="se">Structuring element.</param>
		/// 
		public Closing( short[,] se )
		{
			errosion = new Erosion( se );
			dilatation = new Dilatation(se);
		}

		/// <summary>
		/// Apply filter to an image.
		/// </summary>
		/// 
		/// <param name="image">Source image to apply filter to.</param>
		/// 
		/// <returns>Returns filter's result obtained by applying the filter to
		/// the source image.</returns>
		/// 
		/// <remarks>The method keeps the source image unchanged and returns the
		/// the result of image processing filter as new image.</remarks> 
		///
		public Bitmap Apply( Bitmap image )
		{
			Bitmap tempImage = dilatation.Apply( image );
			Bitmap destImage = errosion.Apply( tempImage );

			tempImage.Dispose( );

			return destImage;
		}

		/// <summary>
		/// Apply filter to an image.
		/// </summary>
		/// 
		/// <param name="imageData">Source image to apply filter to.</param>
		/// 
		/// <returns>Returns filter's result obtained by applying the filter to
		/// the source image.</returns>
		/// 
		/// <remarks>The filter accepts bitmap data as input and returns the result
		/// of image processing filter as new image. The source image data are kept
		/// unchanged.</remarks>
		/// 
		public Bitmap Apply( BitmapData imageData )
		{
			Bitmap tempImage = dilatation.Apply( imageData );
			Bitmap destImage = errosion.Apply( tempImage );

			tempImage.Dispose( );

			return destImage;
		}

        /// <summary>
        /// Apply filter to an image.
        /// </summary>
        /// 
        /// <param name="image">Image to apply filter to.</param>
        /// 
        /// <remarks>The method applies the filter directly to the provided
        /// image.</remarks>
        /// 
        public void ApplyInPlace( Bitmap image )
        {
            dilatation.ApplyInPlace( image );
            errosion.ApplyInPlace( image );
        }

        /// <summary>
        /// Apply filter to an image.
        /// </summary>
        /// 
        /// <param name="imageData">Image to apply filter to.</param>
        /// 
        /// <remarks>The method applies the filter directly to the provided
        /// image data.</remarks>
        /// 
        public void ApplyInPlace( BitmapData imageData )
        {
            dilatation.ApplyInPlace( imageData );
            errosion.ApplyInPlace( imageData );
        }

        /// <summary>
        /// Apply filter to an image or its part.
        /// </summary>
        /// 
        /// <param name="image">Image to apply filter to.</param>
        /// <param name="rect">Image rectangle for processing by the filter.</param>
        /// 
        /// <remarks>The method applies the filter directly to the provided
        /// image.</remarks>
        /// 
        public void ApplyInPlace( Bitmap image, Rectangle rect )
        {
            dilatation.ApplyInPlace( image, rect );
            errosion.ApplyInPlace( image, rect );
        }

        /// <summary>
        /// Apply filter to an image or its part.
        /// </summary>
        /// 
        /// <param name="imageData">Image to apply filter to.</param>
        /// <param name="rect">Image rectangle for processing by the filter.</param>
        /// 
        /// <remarks>The method applies the filter directly to the provided
        /// image data.</remarks>
        /// 
        public void ApplyInPlace( BitmapData imageData, Rectangle rect )
        {
            dilatation.ApplyInPlace( imageData, rect );
            errosion.ApplyInPlace( imageData, rect );
        }

	}
}
