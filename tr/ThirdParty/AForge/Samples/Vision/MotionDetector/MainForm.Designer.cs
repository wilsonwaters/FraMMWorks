namespace MotionDetector
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose( bool disposing )
        {
            if ( disposing && ( components != null ) )
            {
                components.Dispose( );
            }
            base.Dispose( disposing );
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent( )
        {
            this.components = new System.ComponentModel.Container( );
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager( typeof( MainForm ) );
            this.menuMenu = new System.Windows.Forms.MenuStrip( );
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.openJPEGURLToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.openMJPEGURLToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.localVideoCaptureDeviceToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.openVideoFileusingDirectShowToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator( );
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.motionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.noneToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.toolStripMenuItem2 = new System.Windows.Forms.ToolStripSeparator( );
            this.detector1ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.detector2ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.detector3ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.detector4ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.toolStripMenuItem3 = new System.Windows.Forms.ToolStripSeparator( );
            this.highlightMotionRegionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.toolStripMenuItem4 = new System.Windows.Forms.ToolStripSeparator( );
            this.defineMotionregionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem( );
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog( );
            this.timer = new System.Windows.Forms.Timer( this.components );
            this.statusBar = new System.Windows.Forms.StatusStrip( );
            this.fpsLabel = new System.Windows.Forms.ToolStripStatusLabel( );
            this.objectsCountLabel = new System.Windows.Forms.ToolStripStatusLabel( );
            this.panel1 = new System.Windows.Forms.Panel( );
            this.cameraWindow = new MotionDetector.CameraWindow( );
            this.menuMenu.SuspendLayout( );
            this.statusBar.SuspendLayout( );
            this.panel1.SuspendLayout( );
            this.SuspendLayout( );
            // 
            // menuMenu
            // 
            this.menuMenu.Items.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.motionToolStripMenuItem,
            this.helpToolStripMenuItem} );
            this.menuMenu.Location = new System.Drawing.Point( 0, 0 );
            this.menuMenu.Name = "menuMenu";
            this.menuMenu.Size = new System.Drawing.Size( 432, 24 );
            this.menuMenu.TabIndex = 0;
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripMenuItem,
            this.openJPEGURLToolStripMenuItem,
            this.openMJPEGURLToolStripMenuItem,
            this.localVideoCaptureDeviceToolStripMenuItem,
            this.openVideoFileusingDirectShowToolStripMenuItem,
            this.toolStripMenuItem1,
            this.exitToolStripMenuItem} );
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size( 35, 20 );
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.ShortcutKeys = ( (System.Windows.Forms.Keys) ( ( System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O ) ) );
            this.openToolStripMenuItem.Size = new System.Drawing.Size( 250, 22 );
            this.openToolStripMenuItem.Text = "&Open";
            this.openToolStripMenuItem.Click += new System.EventHandler( this.openToolStripMenuItem_Click );
            // 
            // openJPEGURLToolStripMenuItem
            // 
            this.openJPEGURLToolStripMenuItem.Name = "openJPEGURLToolStripMenuItem";
            this.openJPEGURLToolStripMenuItem.Size = new System.Drawing.Size( 250, 22 );
            this.openJPEGURLToolStripMenuItem.Text = "Open JPEG &URL";
            this.openJPEGURLToolStripMenuItem.Click += new System.EventHandler( this.openJPEGURLToolStripMenuItem_Click );
            // 
            // openMJPEGURLToolStripMenuItem
            // 
            this.openMJPEGURLToolStripMenuItem.Name = "openMJPEGURLToolStripMenuItem";
            this.openMJPEGURLToolStripMenuItem.Size = new System.Drawing.Size( 250, 22 );
            this.openMJPEGURLToolStripMenuItem.Text = "Open &MJPEG URL";
            this.openMJPEGURLToolStripMenuItem.Click += new System.EventHandler( this.openMJPEGURLToolStripMenuItem_Click );
            // 
            // localVideoCaptureDeviceToolStripMenuItem
            // 
            this.localVideoCaptureDeviceToolStripMenuItem.Name = "localVideoCaptureDeviceToolStripMenuItem";
            this.localVideoCaptureDeviceToolStripMenuItem.Size = new System.Drawing.Size( 250, 22 );
            this.localVideoCaptureDeviceToolStripMenuItem.Text = "Local &Video Capture Device";
            this.localVideoCaptureDeviceToolStripMenuItem.Click += new System.EventHandler( this.localVideoCaptureDeviceToolStripMenuItem_Click );
            // 
            // openVideoFileusingDirectShowToolStripMenuItem
            // 
            this.openVideoFileusingDirectShowToolStripMenuItem.Name = "openVideoFileusingDirectShowToolStripMenuItem";
            this.openVideoFileusingDirectShowToolStripMenuItem.Size = new System.Drawing.Size( 250, 22 );
            this.openVideoFileusingDirectShowToolStripMenuItem.Text = "Open video &file (using DirectShow)";
            this.openVideoFileusingDirectShowToolStripMenuItem.Click += new System.EventHandler( this.openVideoFileusingDirectShowToolStripMenuItem_Click );
            // 
            // toolStripMenuItem1
            // 
            this.toolStripMenuItem1.Name = "toolStripMenuItem1";
            this.toolStripMenuItem1.Size = new System.Drawing.Size( 247, 6 );
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size( 250, 22 );
            this.exitToolStripMenuItem.Text = "E&xit";
            this.exitToolStripMenuItem.Click += new System.EventHandler( this.exitToolStripMenuItem_Click );
            // 
            // motionToolStripMenuItem
            // 
            this.motionToolStripMenuItem.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.noneToolStripMenuItem,
            this.toolStripMenuItem2,
            this.detector1ToolStripMenuItem,
            this.detector2ToolStripMenuItem,
            this.detector3ToolStripMenuItem,
            this.detector4ToolStripMenuItem,
            this.toolStripMenuItem3,
            this.highlightMotionRegionsToolStripMenuItem,
            this.toolStripMenuItem4,
            this.defineMotionregionsToolStripMenuItem} );
            this.motionToolStripMenuItem.Name = "motionToolStripMenuItem";
            this.motionToolStripMenuItem.Size = new System.Drawing.Size( 51, 20 );
            this.motionToolStripMenuItem.Text = "&Motion";
            this.motionToolStripMenuItem.DropDownOpening += new System.EventHandler( this.motionToolStripMenuItem_DropDownOpening );
            // 
            // noneToolStripMenuItem
            // 
            this.noneToolStripMenuItem.Name = "noneToolStripMenuItem";
            this.noneToolStripMenuItem.Size = new System.Drawing.Size( 199, 22 );
            this.noneToolStripMenuItem.Text = "&None";
            this.noneToolStripMenuItem.Click += new System.EventHandler( this.noneToolStripMenuItem_Click );
            // 
            // toolStripMenuItem2
            // 
            this.toolStripMenuItem2.Name = "toolStripMenuItem2";
            this.toolStripMenuItem2.Size = new System.Drawing.Size( 196, 6 );
            // 
            // detector1ToolStripMenuItem
            // 
            this.detector1ToolStripMenuItem.Name = "detector1ToolStripMenuItem";
            this.detector1ToolStripMenuItem.Size = new System.Drawing.Size( 199, 22 );
            this.detector1ToolStripMenuItem.Text = "Detector &1";
            this.detector1ToolStripMenuItem.Click += new System.EventHandler( this.detector1ToolStripMenuItem_Click );
            // 
            // detector2ToolStripMenuItem
            // 
            this.detector2ToolStripMenuItem.Name = "detector2ToolStripMenuItem";
            this.detector2ToolStripMenuItem.Size = new System.Drawing.Size( 199, 22 );
            this.detector2ToolStripMenuItem.Text = "Detector &2";
            this.detector2ToolStripMenuItem.Click += new System.EventHandler( this.detector2ToolStripMenuItem_Click );
            // 
            // detector3ToolStripMenuItem
            // 
            this.detector3ToolStripMenuItem.Name = "detector3ToolStripMenuItem";
            this.detector3ToolStripMenuItem.Size = new System.Drawing.Size( 199, 22 );
            this.detector3ToolStripMenuItem.Text = "Detector &3";
            this.detector3ToolStripMenuItem.Click += new System.EventHandler( this.detector3ToolStripMenuItem_Click );
            // 
            // detector4ToolStripMenuItem
            // 
            this.detector4ToolStripMenuItem.Name = "detector4ToolStripMenuItem";
            this.detector4ToolStripMenuItem.Size = new System.Drawing.Size( 199, 22 );
            this.detector4ToolStripMenuItem.Text = "Detector &4";
            this.detector4ToolStripMenuItem.Click += new System.EventHandler( this.detector4ToolStripMenuItem_Click );
            // 
            // toolStripMenuItem3
            // 
            this.toolStripMenuItem3.Name = "toolStripMenuItem3";
            this.toolStripMenuItem3.Size = new System.Drawing.Size( 196, 6 );
            // 
            // highlightMotionRegionsToolStripMenuItem
            // 
            this.highlightMotionRegionsToolStripMenuItem.Checked = true;
            this.highlightMotionRegionsToolStripMenuItem.CheckState = System.Windows.Forms.CheckState.Checked;
            this.highlightMotionRegionsToolStripMenuItem.Name = "highlightMotionRegionsToolStripMenuItem";
            this.highlightMotionRegionsToolStripMenuItem.Size = new System.Drawing.Size( 199, 22 );
            this.highlightMotionRegionsToolStripMenuItem.Text = "Highlight motion regions";
            this.highlightMotionRegionsToolStripMenuItem.Click += new System.EventHandler( this.highlightMotionRegionsToolStripMenuItem_Click );
            // 
            // toolStripMenuItem4
            // 
            this.toolStripMenuItem4.Name = "toolStripMenuItem4";
            this.toolStripMenuItem4.Size = new System.Drawing.Size( 196, 6 );
            // 
            // defineMotionregionsToolStripMenuItem
            // 
            this.defineMotionregionsToolStripMenuItem.Name = "defineMotionregionsToolStripMenuItem";
            this.defineMotionregionsToolStripMenuItem.Size = new System.Drawing.Size( 199, 22 );
            this.defineMotionregionsToolStripMenuItem.Text = "Define motion &regions";
            this.defineMotionregionsToolStripMenuItem.Click += new System.EventHandler( this.defineMotionregionsToolStripMenuItem_Click );
            // 
            // helpToolStripMenuItem
            // 
            this.helpToolStripMenuItem.DropDownItems.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.aboutToolStripMenuItem} );
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            this.helpToolStripMenuItem.Size = new System.Drawing.Size( 40, 20 );
            this.helpToolStripMenuItem.Text = "&Help";
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size( 114, 22 );
            this.aboutToolStripMenuItem.Text = "&About";
            this.aboutToolStripMenuItem.Click += new System.EventHandler( this.aboutToolStripMenuItem_Click );
            // 
            // openFileDialog
            // 
            this.openFileDialog.Filter = "AVI files (*.avi)|*.avi|All files (*.*)|*.*";
            this.openFileDialog.Title = "Opem movie";
            // 
            // timer
            // 
            this.timer.Interval = 1000;
            this.timer.Tick += new System.EventHandler( this.timer_Tick );
            // 
            // statusBar
            // 
            this.statusBar.Items.AddRange( new System.Windows.Forms.ToolStripItem[] {
            this.fpsLabel,
            this.objectsCountLabel} );
            this.statusBar.Location = new System.Drawing.Point( 0, 334 );
            this.statusBar.Name = "statusBar";
            this.statusBar.Size = new System.Drawing.Size( 432, 22 );
            this.statusBar.TabIndex = 3;
            // 
            // fpsLabel
            // 
            this.fpsLabel.AutoSize = false;
            this.fpsLabel.BorderSides = ( (System.Windows.Forms.ToolStripStatusLabelBorderSides) ( ( ( ( System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top )
                        | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right )
                        | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom ) ) );
            this.fpsLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this.fpsLabel.Name = "fpsLabel";
            this.fpsLabel.Size = new System.Drawing.Size( 150, 17 );
            this.fpsLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // objectsCountLabel
            // 
            this.objectsCountLabel.BorderSides = ( (System.Windows.Forms.ToolStripStatusLabelBorderSides) ( ( ( ( System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top )
                        | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right )
                        | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom ) ) );
            this.objectsCountLabel.BorderStyle = System.Windows.Forms.Border3DStyle.SunkenInner;
            this.objectsCountLabel.Name = "objectsCountLabel";
            this.objectsCountLabel.Size = new System.Drawing.Size( 267, 17 );
            this.objectsCountLabel.Spring = true;
            this.objectsCountLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // panel1
            // 
            this.panel1.Controls.Add( this.cameraWindow );
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point( 0, 24 );
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size( 432, 310 );
            this.panel1.TabIndex = 4;
            // 
            // cameraWindow
            // 
            this.cameraWindow.BackColor = System.Drawing.SystemColors.ControlDarkDark;
            this.cameraWindow.Camera = null;
            this.cameraWindow.Location = new System.Drawing.Point( 52, 28 );
            this.cameraWindow.Name = "cameraWindow";
            this.cameraWindow.Size = new System.Drawing.Size( 320, 240 );
            this.cameraWindow.TabIndex = 1;
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF( 6F, 13F );
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size( 432, 356 );
            this.Controls.Add( this.panel1 );
            this.Controls.Add( this.statusBar );
            this.Controls.Add( this.menuMenu );
            this.Icon = ( (System.Drawing.Icon) ( resources.GetObject( "$this.Icon" ) ) );
            this.MainMenuStrip = this.menuMenu;
            this.Name = "MainForm";
            this.Text = "Motion Detector";
            this.SizeChanged += new System.EventHandler( this.MainForm_SizeChanged );
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler( this.MainForm_FormClosing );
            this.menuMenu.ResumeLayout( false );
            this.menuMenu.PerformLayout( );
            this.statusBar.ResumeLayout( false );
            this.statusBar.PerformLayout( );
            this.panel1.ResumeLayout( false );
            this.ResumeLayout( false );
            this.PerformLayout( );

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuMenu;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem motionToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.OpenFileDialog openFileDialog;
        private CameraWindow cameraWindow;
        private System.Windows.Forms.Timer timer;
        private System.Windows.Forms.StatusStrip statusBar;
        private System.Windows.Forms.ToolStripStatusLabel fpsLabel;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ToolStripMenuItem openJPEGURLToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openMJPEGURLToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem noneToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem2;
        private System.Windows.Forms.ToolStripMenuItem detector1ToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem detector2ToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem detector3ToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem3;
        private System.Windows.Forms.ToolStripMenuItem highlightMotionRegionsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem localVideoCaptureDeviceToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openVideoFileusingDirectShowToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem detector4ToolStripMenuItem;
        private System.Windows.Forms.ToolStripStatusLabel objectsCountLabel;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem4;
        private System.Windows.Forms.ToolStripMenuItem defineMotionregionsToolStripMenuItem;
    }
}

