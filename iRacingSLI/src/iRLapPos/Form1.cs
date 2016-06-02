using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using iRSDKSharp;
using iRacingSdkWrapper;
using iRacingSdkWrapper.Bitfields;

namespace iRLapPos
{
    /// <summary>
    /// Represents a driver in the current session.
    /// </summary>
    public class Driver
    {
        public Driver()
        {
        }

        /// <summary>
        /// The identifier (CarIdx) of this driver (unique to this session)
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// The current position of the driver
        /// </summary>
        public int Position { get; set; }

        /// <summary>
        /// The name of the driver
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// The customer ID (custid) of the driver, used to identify GoT drivers.
        /// </summary>
        public int CustomerId { get; set; }

        /// <summary>
        /// The car number of this driver
        /// </summary>
        public string Number { get; set; }

        /// <summary>
        /// A unique identifier for the car class this driver is using
        /// </summary>
        public int ClassId { get; set; }

        /// <summary>
        /// The name of the car of this driver
        /// </summary>
        public string CarPath { get; set; }

        /// <summary>
        /// The relative speed of this class in a multiclass session
        /// </summary>
        public int CarClassRelSpeed { get; set; }

        /// <summary>
        /// Used to determine if a driver is in the pits, off or on track
        /// </summary>
        public TrackSurfaces TrackSurface { get; set; }

        /// <summary>
        /// Whether or not the driver is currently in or approaching the pit stall
        /// </summary>
        public bool IsInPits
        {
            get { return this.TrackSurface == TrackSurfaces.AproachingPits || this.TrackSurface == TrackSurfaces.InPitStall; }
        }

        /// <summary>
        /// The lap this driver is currently in
        /// </summary>
        public int Lap { get; set; }

        /// <summary>
        /// The distance along the current lap of this driver (in percentage)
        /// </summary>
        public float LapDistance { get; set; }

        /// <summary>
        /// The relative distance between you and this driver (in percentage).
        /// </summary>
        public float RelativeLapDistance { get; set; }

        /// <summary>
        /// The fastest lap time of this driver
        /// </summary>
        public float FastestLapTime { get; set; }

        /// <summary>
        /// The last lap time of this driver
        /// </summary>
        public float LastLapTime { get; set; }

        /// <summary>
        /// The iRating of this driver
        /// </summary>
        public int Rating { get; set; }
    }

    public partial class Form1 : Form
    {
        private SdkWrapper wrapper;
        int currentSessionNum = 0;
        int RaceLap = 0;
        int currentLaps = 0;
        uint Flag=0;

        public Form1()
        {
            InitializeComponent();
            // Create a new instance of the SdkWrapper object
            wrapper = new SdkWrapper();

            // Tell it to raise events on the current thread (don't worry if you don't know what a thread is)
            wrapper.EventRaiseType = SdkWrapper.EventRaiseTypes.CurrentThread;

            // Only update telemetry 10 times per second
            wrapper.TelemetryUpdateFrequency = 10;

            // Attach some useful events so you can respond when they get raised
            wrapper.Connected += wrapper_Connected;
            wrapper.Disconnected += wrapper_Disconnected;
            //wrapper.SessionInfoUpdated += wrapper_SessionInfoUpdated;
            wrapper.TelemetryUpdated += wrapper_TelemetryUpdated;

            UpdateTimer.Tick += new EventHandler(this.UpdateTimerTick);
        }

        private void StatusChanged()
        {
            if (wrapper.IsConnected)
            {
                if (wrapper.IsRunning)
                {
                    //statusLabel.Text = "Status: connected!";
                }
                else
                {
                    //statusLabel.Text = "Status: disconnected.";
                }
            }
            else
            {
                if (wrapper.IsRunning)
                {
                    //statusLabel.Text = "Status: disconnected, waiting for sim...";
                }
                else
                {
                    //statusLabel.Text = "Status: disconnected";
                }
            }
        }

        private void wrapper_Connected(object sender, EventArgs e)
        {
            this.StatusChanged();
        }

        private void wrapper_Disconnected(object sender, EventArgs e)
        {
            this.StatusChanged();
        }

        private void wrapper_TelemetryUpdated(object sender, SdkWrapper.TelemetryUpdatedEventArgs e)
        {
            // Besides the driver details found in the session info, there's also things in the telemetry
            // that are properties of a driver, such as their lap, lap distance, track surface, distance relative
            // to yourself and more.
            // We update the existing list of drivers with the telemetry values here.

            // If we are currently renewing the drivers list it makes little sense to update the existing drivers
            // because they will change anyway
            //if (isUpdatingDrivers) return;

            // Store the current session number so we know which session to read 
            // There can be multiple sessions in a server (practice, Q, race, or warmup, race, etc).
            currentSessionNum = e.TelemetryInfo.SessionNum.Value;
            RaceLap = e.TelemetryInfo.RaceLaps.Value;
            currentLaps = e.TelemetryInfo.Lap.Value;
            
            //e.TelemetryInfo.
            //this.UpdateDriversTelemetry(e.TelemetryInfo);
        }
        
        private void UpdateTimerTick(object sender,EventArgs e)
        {
            //Todo:
        }

    }
}
