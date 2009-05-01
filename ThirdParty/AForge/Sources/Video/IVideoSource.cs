// AForge Video Library
// AForge.NET framework
//
// Copyright � Andrew Kirillov, 2007
// andrew.kirillov@gmail.com
//

namespace AForge.Video
{
	using System;

	/// <summary>
	/// Video source interface.
	/// </summary>
    /// 
    /// <remarks>The interface describes common methods for all
    /// video sources.</remarks>
    /// 
	public interface IVideoSource
	{
		/// <summary>
		/// New frame event.
		/// </summary>
        /// 
        event NewFrameEventHandler NewFrame;

        /// <summary>
        /// Video source error event.
        /// </summary>
        /// 
        /// <remarks>The event is used to notify client about any type error occurred in
        /// video source object, for example exceptions.</remarks>
        /// 
        event VideoSourceErrorEventHandler VideoSourceError;

		/// <summary>
		/// Video source.
		/// </summary>
        /// 
        /// <remarks>The meaning of the property depends on particular video source.
        /// Depending on video source it ma be file name, URL or any other string
        /// describing the video source.</remarks>
        /// 
		string Source{ get; set; }

		/// <summary>
		/// Received frames count.
		/// </summary>
        /// 
        /// <remarks>Number of frames the video source provided from the moment of the last
        /// access to the property.
        /// </remarks>
        /// 
		int FramesReceived{ get; }

		/// <summary>
		/// Received bytes count.
		/// </summary>
        /// 
        /// <remarks>Number of bytes the video source provided from the moment of the last
		/// access to the property.
        /// </remarks>
        /// 
		int BytesReceived{ get; }

		/// <summary>
		/// User data.
		/// </summary>
        /// 
        /// <remarks>The property allows to associate user data with video source object.</remarks>
        /// 
		object UserData{ get; set; }

		/// <summary>
		/// State of the video source.
		/// </summary>
        /// 
        /// <remarks>Current state of video source object.</remarks>
        /// 
		bool IsRunning{ get; }

		/// <summary>
		/// Start video source.
		/// </summary>
        /// 
        /// <remarks>Start video source and return execution to caller. Video source
        /// object creates background thread and notifies about new frames with the
        /// help of <see cref="NewFrame"/> event.</remarks>
        /// 
		void Start( );

		/// <summary>
		/// Signal video source to stop its work.
		/// </summary>
        /// 
        /// <remarks>Signals video source to stop its background thread, stop to
        /// provide new frames and free resources.</remarks>
        /// 
		void SignalToStop( );

		/// <summary>
		/// Wait for video source has stopped.
		/// </summary>
        /// 
        /// <remarks>Waits for source stopping after it was signalled to stop using
        /// <see cref="SignalToStop"/> method.</remarks>
        /// 
		void WaitForStop( );

		/// <summary>
		/// Stop video source.
		/// </summary>
        /// 
        /// <remarks>Stops video source aborting its thread.</remarks>
        /// 
		void Stop( );
	}
}
