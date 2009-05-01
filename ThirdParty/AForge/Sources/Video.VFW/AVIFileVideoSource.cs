// AForge Video for Windows Library
// AForge.NET framework
//
// Copyright � Andrew Kirillov, 2007
// andrew.kirillov@gmail.com
//
namespace AForge.Video.VFW
{
    using System;
	using System.Drawing;
	using System.Drawing.Imaging;
	using System.IO;
	using System.Threading;

    using AForge.Video;

	/// <summary>
	/// AVI file video source.
	/// </summary>
    /// 
    /// <remarks><para>The video source read AVI files using Video for Windows.</para>
    /// <para>Sample usage:</para>
    /// <code>
    /// // create AVI file video source
    /// AVIFileVideoSource source = new AVIFileVideoSource( "some file" );
    /// // set event handlers
    /// source.NewFrame += new NewFrameEventHandler( video_NewFrame );
    /// // start the video source
    /// source.Start( );
    /// // ...
    /// // signal to stop
    /// source.SignalToStop( );
    /// </code>
    /// </remarks>
    /// 
	public class AVIFileVideoSource : IVideoSource
	{
        // video file name
		private string source;
        // user data associated with the video source
		private object userData = null;
        // received frames count
		private int framesReceived;
        // frame interval in milliseconds
        private int frameInterval = 0;
        // get frame interval from source or use manually specified
        private bool frameIntervalFromSource = false;

		private Thread thread = null;
		private ManualResetEvent stopEvent = null;

        /// <summary>
        /// New frame event.
        /// </summary>
        /// 
        /// <remarks>Notifies client about new available frame from video source.</remarks>
        /// 
        public event NewFrameEventHandler NewFrame;

        /// <summary>
        /// Video source error event.
        /// </summary>
        /// 
        /// <remarks>The event is used to notify client about any type error occurred in
        /// video source object, for example exceptions.</remarks>
        /// 
        public event VideoSourceErrorEventHandler VideoSourceError;

        /// <summary>
        /// Frame interval.
        /// </summary>
        /// 
        /// <remarks>The property sets the interval in milliseconds betwen frames. If the property is
        /// set to 100, then the d��ired frame rate will be 10 frames per second. Default value is 0 -
        /// get new frames as fast as possible.</remarks>
        /// 
        public int FrameInterval
        {
            get { return frameInterval; }
            set { frameInterval = value; }
        }

        /// <summary>
        /// Get frame interval from source or use manually specified.
        /// </summary>
        /// 
        public bool FrameIntervalFromSource
        {
            get { return frameIntervalFromSource; }
            set { frameIntervalFromSource = value; }
        }

        /// <summary>
        /// Video source.
        /// </summary>
        /// 
        /// <remarks>Video file name.</remarks>
        /// 
        public virtual string Source
		{
			get { return source; }
			set { source = value; }
		}

        /// <summary>
        /// Received frames count.
        /// </summary>
        /// 
        /// <remarks>Number of frames the video source provided from the moment of the last
        /// access to the property.
        /// </remarks>
        /// 
        public int FramesReceived
		{
			get
			{
				int frames = framesReceived;
				framesReceived = 0;
				return frames;
			}
		}

        /// <summary>
        /// Received bytes count.
        /// </summary>
        /// 
        /// <remarks>The property is not supported by the class. It always equals to 0.</remarks>
        /// 
        public int BytesReceived
		{
			get { return 0; }
		}

        /// <summary>
        /// User data.
        /// </summary>
        /// 
        /// <remarks>The property allows to associate user data with video source object.</remarks>
        /// 
        public object UserData
		{
			get { return userData; }
			set { userData = value; }
		}

        /// <summary>
        /// State of the video source.
        /// </summary>
        /// 
        /// <remarks>Current state of video source object.</remarks>
        /// 
        public bool IsRunning
		{
			get
			{
				if ( thread != null )
				{
                    // check thread status
                    if ( thread.Join( 0 ) == false )
						return true;

					// the thread is not running, so free resources
					Free( );
				}
				return false;
			}
		}

        /// <summary>
        /// Initializes a new instance of the <see cref="AVIFileVideoSource"/> class.
        /// </summary>
        /// 
		public AVIFileVideoSource( ) { }

        /// <summary>
        /// Initializes a new instance of the <see cref="AVIFileVideoSource"/> class.
        /// </summary>
        /// 
        /// <param name="source">Video file name.</param>
        /// 
        public AVIFileVideoSource( string source )
        {
            this.source = source;
        }

        /// <summary>
        /// Start video source.
        /// </summary>
        /// 
        /// <remarks>Start video source and return execution to caller. Video source
        /// object creates background thread and notifies about new frames with the
        /// help of <see cref="NewFrame"/> event.</remarks>
        /// 
        public void Start( )
		{
			if ( thread == null )
			{
                // check source
                if ( ( source == null ) || ( source == string.Empty ) )
                    throw new ArgumentException( "Video source is not specified" );
                
                framesReceived = 0;

				// create events
				stopEvent = new ManualResetEvent( false );
				
				// create and start new thread
				thread = new Thread( new ThreadStart( WorkerThread ) );
				thread.Name = source;
				thread.Start( );
			}
		}

        /// <summary>
        /// Signal video source to stop its work.
        /// </summary>
        /// 
        /// <remarks>Signals video source to stop its background thread, stop to
        /// provide new frames and free resources.</remarks>
        /// 
        public void SignalToStop( )
		{
			// stop thread
			if ( thread != null )
			{
				// signal to stop
				stopEvent.Set( );
			}
		}

        /// <summary>
        /// Wait for video source has stopped.
        /// </summary>
        /// 
        /// <remarks>Waits for source stopping after it was signalled to stop using
        /// <see cref="SignalToStop"/> method.</remarks>
        /// 
        public void WaitForStop( )
		{
			if ( thread != null )
			{
				// wait for thread stop
				thread.Join( );

				Free( );
			}
		}

        /// <summary>
        /// Stop video source.
        /// </summary>
        /// 
        /// <remarks>Stops video source aborting its thread.</remarks>
        /// 
        public void Stop( )
		{
			if ( this.IsRunning )
			{
				thread.Abort( );
				WaitForStop( );
			}
		}

        /// <summary>
        /// Free resource.
        /// </summary>
        /// 
        private void Free( )
		{
			thread = null;

			// release events
			stopEvent.Close( );
			stopEvent = null;
		}

        /// <summary>
        /// Worker thread.
        /// </summary>
        /// 
        public void WorkerThread( )
		{
            // AVI reader
			AVIReader aviReader = new AVIReader( );

			try
			{
				// open file
				aviReader.Open( source );

                // stop positions
                int stopPosition = aviReader.Start + aviReader.Length;

                // frame interval
                int interval = ( frameIntervalFromSource ) ? (int) ( 1000 / aviReader.FrameRate ) : frameInterval;

				while ( true )
				{
					// start time
					DateTime start = DateTime.Now;

					// get next frame
					Bitmap bitmap = aviReader.GetNextFrame( );

					framesReceived++;

					// need to stop ?
					if ( stopEvent.WaitOne( 0, false ) )
						break;

					if ( NewFrame != null )
						NewFrame( this, new NewFrameEventArgs( bitmap ) );

					// free image
					bitmap.Dispose( );

                    // check current position
                    if ( aviReader.Position >= stopPosition )
                        break;

                    // wait for a while ?
                    if ( interval > 0 )
                    {
                        // get frame extract duration
                        TimeSpan span = DateTime.Now.Subtract( start );

                        // miliseconds to sleep
                        int msec = interval - (int) span.TotalMilliseconds;

                        while ( ( msec > 0 ) && ( stopEvent.WaitOne( 0, true ) == false ) )
                        {
                            // sleeping ...
                            Thread.Sleep( ( msec < 100 ) ? msec : 100 );
                            msec -= 100;
                        }
                    }
				}
			}
			catch ( Exception exception )
			{
                // provide information to clients
                if ( VideoSourceError != null )
                {
                    VideoSourceError( this, new VideoSourceErrorEventArgs( exception.Message ) );
                }
			}

			aviReader.Dispose( );
			aviReader = null;
		}
	}
}
