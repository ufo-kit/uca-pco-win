/**
Notes:
    PCO SDK uses windows specific data types in its header files and documentation.
    This program has glib data types. For ex. to a pco library function that requires WORD, 
    relevant guint16 variable is initialized and given.

    minwindef.h typedef's windows specific datatypes to generic datatypes
**/

#include <gmodule.h>
#include <gio/gio.h>
#include <string.h>
#include <math.h>
#include <minwindef.h>
#include <sc2_SDKStructures.h>
#include <sc2_defs.h>
#include <PCO_err.h>
#include <PCO_errt.h>
#include "uca-pco-win-camera.h"
#include "uca-pco-enums.h"

#define TRIGGER_MODE_AUTOTRIGGER        0x0000
#define TRIGGER_MODE_SOFTWARETRIGGER    0x0001
#define TRIGGER_MODE_EXTERNALTRIGGER    0x0002

#define UCA_PCOWIN_CAMERA_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UCA_TYPE_PCOWIN_CAMERA, UcaPcowinCameraPrivate))

static void uca_pcowin_initable_iface_init (GInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (UcaPcowinCamera, uca_pcowin_camera, UCA_TYPE_CAMERA,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                uca_pcowin_initable_iface_init))

                                                
                                                

// Additional properties for PCO Cameras
enum {
    PROP_SENSOR_EXTENDED = N_BASE_PROPERTIES,
    PROP_SENSOR_WIDTH_EXTENDED,
    PROP_SENSOR_HEIGHT_EXTENDED,
    PROP_SENSOR_TEMPERATURE,
    PROP_SENSOR_PIXELRATES,
    PROP_SENSOR_PIXELRATE,
    //PROP_SENSOR_ADCS,
    //PROP_SENSOR_MAX_ADCS,
    //PROP_HAS_DOUBLE_IMAGE_MODE,
    //PROP_DOUBLE_IMAGE_MODE,
    PROP_OFFSET_MODE,
    PROP_RECORD_MODE,
    PROP_STORAGE_MODE,
    PROP_ACQUIRE_MODE,
    //PROP_FAST_SCAN,
    PROP_COOLING_POINT,
    PROP_COOLING_POINT_MIN,
    PROP_COOLING_POINT_MAX,
    PROP_COOLING_POINT_DEFAULT,
    //PROP_NOISE_FILTER,
    //PROP_TIMESTAMP_MODE,
    PROP_VERSION,
    //PROP_EDGE_GLOBAL_SHUTTER,
    //PROP_FRAME_GRABBER_TIMEOUT,
    N_PROPERTIES
};

static gint base_overrideables[] = {
    PROP_NAME,
    PROP_SENSOR_WIDTH,
    PROP_SENSOR_HEIGHT,
    PROP_SENSOR_PIXEL_WIDTH,
    PROP_SENSOR_PIXEL_HEIGHT,
    PROP_SENSOR_BITDEPTH,
    PROP_SENSOR_HORIZONTAL_BINNING,
    PROP_SENSOR_VERTICAL_BINNING,
    PROP_EXPOSURE_TIME,
    PROP_FRAMES_PER_SECOND,
    PROP_TRIGGER_SOURCE,
    PROP_ROI_X,
    PROP_ROI_Y,
    PROP_ROI_WIDTH,
    PROP_ROI_HEIGHT,
    PROP_ROI_WIDTH_MULTIPLIER,
    PROP_ROI_HEIGHT_MULTIPLIER,
    PROP_HAS_STREAMING,
    PROP_HAS_CAMRAM_RECORDING,
    PROP_RECORDED_FRAMES,
    PROP_IS_RECORDING,
    0
};

// PCO Camera Structure with all available properties
static GParamSpec *pco_properties[N_PROPERTIES] = { NULL };

GQuark uca_pcowin_camera_error_quark()
{
    return g_quark_from_static_string("uca-pcowin-camera-error-quark");
}

struct _UcaPcowinCameraPrivate {
    
    HANDLE pcoHandle;
    
    GError *construct_error;
    guint current_frame;
    
    PCO_General strGeneral;
    PCO_CameraType strCamType;
    PCO_Sensor strSensor;
    PCO_Description strDescription;
    PCO_Timing strTiming;
    PCO_Storage strStorage;
    PCO_Recording strRecording;

    guint16 roi_x, roi_y;
    guint16 roi_width, roi_height;
    
};

map_timebase(short timebase)
{
    switch(timebase){
        case 0x0:
            return 1e-9;
        case 0x1:
            return 1e-6;
        case 0x2:
        default:
            return 1e-3;
    }
}

static gpointer
pcowin_grab_func(gpointer data)
{

}

static void
uca_pcowin_camera_start_recording(UcaCamera *camera, GError **error)
{

}

static void
uca_pcowin_camera_stop_recording(UcaCamera *camera, GError **error)
{

}

static void
uca_pcowin_camera_start_readout(UcaCamera *camera, GError **error)
{
}

static void
uca_pcowin_camera_stop_readout(UcaCamera *camera, GError **error)
{
}

static void
uca_pcowin_camera_trigger (UcaCamera *camera, GError **error)
{
}

static gboolean
uca_pcowin_camera_grab (UcaCamera *camera, gpointer data, GError **error)
{

}

static gboolean
uca_pcowin_camera_readout (UcaCamera *camera, gpointer data, guint index, GError **error)
{
    
}

static void
uca_pcowin_camera_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{

}

static void
uca_pcowin_camera_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    UcaPcowinCameraPrivate *priv;
    int libraryErrors;
    
    priv = UCA_PCOWIN_CAMERA_GET_PRIVATE(object);
    
    switch (property_id) {
        case PROP_SENSOR_EXTENDED:
        {
            guint16 format;
            libraryErrors = PCO_GetSensorFormat(priv->pcoHandle,&format);
            g_value_set_boolean(value, (format==1));
        }
            break;
        case PROP_SENSOR_WIDTH_EXTENDED:
            g_value_set_uint(value, priv->strDescription.wMaxHorzResExtDESC < priv->strDescription.wMaxHorzResStdDESC ? priv->strDescription.wMaxHorzResStdDESC : priv->strDescription.wMaxHorzResExtDESC);
            break;
        case PROP_SENSOR_HEIGHT_EXTENDED:
            g_value_set_uint(value, priv->strDescription.wMaxVertResExtDESC < priv->strDescription.wMaxVertResStdDESC ? priv->strDescription.wMaxVertResStdDESC : priv->strDescription.wMaxVertResExtDESC);
            break;
        case PROP_SENSOR_TEMPERATURE:
        {
            short ccdTemp, camTemp, powTemp;
            libraryErrors = PCO_GetTemperature(priv->pcoHandle,&ccdTemp, &camTemp, &powTemp);
            g_value_set_double(value,ccdTemp/10);
        }
            break;
        case PROP_SENSOR_PIXELRATES:
            //Ask
            break;
        case PROP_SENSOR_PIXELRATE:
        {
            guint32 pixelrate; // pixelrate in Hz
            libraryErrors = PCO_GetPixelRate(priv->pcoHandle, &pixelrate);
            g_value_set_uint(value, pixelrate);
        }
            break;
        case PROP_OFFSET_MODE:
            {
                guint16 offsetRegulation;
                libraryErrors = PCO_GetOffsetMode(priv->pcoHandle, &offsetRegulation);
                g_value_set_boolean(value, offsetRegulation ? TRUE: FALSE);
            }
            break;
        case PROP_RECORD_MODE:
            {
                // Only available if the storage mode is set to recorder
                guint16 recorderSubmode;
                libraryErrors = PCO_GetRecorderSubmode(priv->pcoHandle, &recorderSubmode);

                switch(recorderSubmode)
                {
                    case RECORDER_SUBMODE_SEQUENCE:
                        g_value_set_enum(value,UCA_PCO_CAMERA_RECORD_MODE_SEQUENCE);
                        break;
                    case RECORDER_SUBMODE_RINGBUFFER:
                        g_value_set_enum(value, UCA_PCO_CAMERA_RECORD_MODE_RING_BUFFER);
                        break;
                }
            }
            break;
        case PROP_STORAGE_MODE:
            {
                guint16 storageMode;
                libraryErrors = PCO_GetStorageMode(priv->pcoHandle, &storageMode);

                switch (storageMode)
                {
                    case STORAGE_MODE_RECORDER:
                        g_value_set_enum (value, UCA_PCO_CAMERA_STORAGE_MODE_RECORDER);
                        break;
                    case STORAGE_MODE_FIFO_BUFFER:
                        g_value_set_enum (value, UCA_PCO_CAMERA_STORAGE_MODE_FIFO_BUFFER);
                        break;
                }
            }
            break;
        case PROP_ACQUIRE_MODE:
            {
                guint16 acqMode;
                libraryErrors = PCO_GetAcquireMode(priv->pcoHandle, &acqMode);

                switch (acqMode)
                {
                    case ACQUIRE_MODE_AUTO:
                        g_value_set_enum (value, UCA_PCO_CAMERA_ACQUIRE_MODE_AUTO);
                        break;
                    case ACQUIRE_MODE_EXTERNAL:
                        g_value_set_enum (value, UCA_PCO_CAMERA_ACQUIRE_MODE_EXTERNAL);
                        break;
                }
            }
            break;
        case PROP_COOLING_POINT:
            {
                gint16 coolingSetPoint;
                libraryErrors = PCO_GetCoolingSetpointTemperature(priv->pcoHandle, &coolingSetPoint);
                g_value_set_int(value, coolingSetPoint);
            }
            break;

        case PROP_COOLING_POINT_MIN:
            g_value_set_int(value,priv->strDescription.sMinCoolSetDESC);
            break;
        case PROP_COOLING_POINT_MAX:
            g_value_set_int(value,priv->strDescription.sMaxCoolSetDESC);
            break;
        case PROP_COOLING_POINT_DEFAULT:
            g_value_set_int(value,priv->strDescription.sDefaultCoolSetDESC);
            break;
        // Generic UCA Properties
        case PROP_SENSOR_WIDTH:
            g_value_set_uint(value, priv->strDescription.wMaxHorzResStdDESC);
            break;
        case PROP_SENSOR_HEIGHT:
            g_value_set_uint(value, priv->strDescription.wMaxVertResStdDESC);
            break;
        case PROP_SENSOR_PIXEL_WIDTH:
            // Ask
            break;
        case PROP_SENSOR_PIXEL_HEIGHT:
            // Ask
            break;
        case PROP_SENSOR_HORIZONTAL_BINNING:
            g_value_set_uint(value, priv->strSensor.wBinHorz);
            break;
        case PROP_SENSOR_VERTICAL_BINNING:
            g_value_set_uint(value, priv->strSensor.wBinVert);
            break;
        case PROP_SENSOR_BITDEPTH:
            // Ask
            break;
        case PROP_EXPOSURE_TIME:
        {
            guint32 delay, exposure;
            short delayTimeBase, exposureTimeBase;
            libraryErrors = PCO_GetDelayExposureTime(priv->pcoHandle, &delay, &exposure, &delayTimeBase, &exposureTimeBase);
            g_value_set_double(value, map_timebase(exposureTimeBase) * exposure);
        }
            break;
        case PROP_FRAMES_PER_SECOND:
        {
            // Works for dimax and edge only
            guint16 framerateStatus;
            guint32 framerate, framerateExposure;
            libraryErrors = PCO_GetFrameRate(priv->pcoHandle, &framerateStatus, &framerate, &framerateExposure);
            g_value_set_double(value, framerate / 1000); //mHz to Hz
        }
            break;
        case PROP_HAS_STREAMING:
            g_value_set_boolean(value, TRUE); 
            break;

        case PROP_HAS_CAMRAM_RECORDING:
            // @ToDo. PCI Dimax has camRAM, but few PCO cameras do not. Replace this for expanding this plugin for other cameras
            g_value_set_boolean(value, TRUE);
            break;

        case PROP_RECORDED_FRAMES:
        {
            // This number is dynamic if the camera is running in recorder mode or in FIFO buffer mode. Result is accurate when recording is stopped
            guint16 segment;
            guint32 validImages, maxImages;
            libraryErrors = PCO_GetActiveRamSegment(priv->pcoHandle, &segment);
            libraryErrors = PCO_GetNumberOfImagesInSegment(priv->pcoHandle, segment, &validImages, &maxImages);
            g_value_set_uint(value, validImages);
        }
            break;      
        case PROP_TRIGGER_SOURCE:
        {
            guint16 triggerMode;
            libraryErrors = PCO_GetTriggerMode(priv->pcoHandle, &triggerMode);

            switch (triggerMode) {
                case TRIGGER_MODE_AUTOTRIGGER:
                    g_value_set_enum(value, UCA_CAMERA_TRIGGER_SOURCE_AUTO);
                    break;
                case TRIGGER_MODE_SOFTWARETRIGGER:
                    g_value_set_enum(value, UCA_CAMERA_TRIGGER_SOURCE_SOFTWARE);
                    break;
                case TRIGGER_MODE_EXTERNALTRIGGER:
                    g_value_set_enum(value, UCA_CAMERA_TRIGGER_SOURCE_EXTERNAL);
                    break;
                default:
                    g_warning("Trigger mode provided by camera cannot be handled");
            }
        }
            break;
        case PROP_ROI_X:
            g_value_set_uint(value, priv->roi_x);
            break;
        case PROP_ROI_Y:
            g_value_set_uint(value, priv->roi_y);
            break;
        case PROP_ROI_WIDTH:
            g_value_set_uint(value, priv->roi_width);
            break;
        case PROP_ROI_HEIGHT:
            g_value_set_uint(value, priv->roi_height);
            break;
        case PROP_ROI_WIDTH_MULTIPLIER:
            g_value_set_uint(value, priv->strDescription.wRoiHorStepsDESC);
            break;
        case PROP_ROI_HEIGHT_MULTIPLIER:
            g_value_set_uint(value, priv->strDescription.wRoiVertStepsDESC);
            break;
        case PROP_NAME:
            g_value_set_string (value, "pco for windows");
            break;
        case PROP_VERSION:
            {
                // @ToDo. convert hardware and firmware version to dotted notation of major number and minor number
                g_value_set_string(value, g_strdup_printf ("%u, %u, %u", priv->strCamType.dwSerialNumber, priv->strCamType.dwHWVersion, priv->strCamType.dwFWVersion));
            }
            break;
        case PROP_IS_RECORDING:
            {
                // @ToDo. Find if camera is recording. PCO_GetRecordingState maybe ??
                g_value_set_boolean(value,FALSE);
            }
            break;
        default:
            g_warning("Undefined Property");
    }

    if(libraryErrors)
    {
        g_set_error (&priv->construct_error,
                     UCA_PCOWIN_CAMERA_ERROR, UCA_PCOWIN_CAMERA_ERROR_GETTER,
                     "Failed to get property %s from library. Here's error code 0x%X for enquiring minds", pco_properties[property_id]->name ,libraryErrors);
    }
}

static void
uca_pcowin_camera_finalize(GObject *object)
{
    UcaPcowinCameraPrivate *priv = UCA_PCOWIN_CAMERA_GET_PRIVATE(object);
    
    g_clear_error (&priv->construct_error);
    
    PCO_CloseCamera (priv->pcoHandle);

    G_OBJECT_CLASS (uca_pcowin_camera_parent_class)->finalize(object);
}

static gboolean
uca_pcowin_camera_initable_init (GInitable *initable,
                               GCancellable *cancellable,
                               GError **error)
{
    UcaPcowinCameraPrivate *priv;
    gdouble step_size;

    g_return_val_if_fail (UCA_IS_PCOWIN_CAMERA (initable), FALSE);
    
    if (cancellable != NULL) {
        g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                             "Cancellable initialization not supported");
        return FALSE;
    }
    
    priv = UCA_PCOWIN_CAMERA_GET_PRIVATE (UCA_PCOWIN_CAMERA (initable));
    if (priv->construct_error != NULL) {
        if (error)
            *error = g_error_copy (priv->construct_error);

        return FALSE;
    }

    return TRUE;
}

static void
uca_pcowin_initable_iface_init (GInitableIface *iface)
{
    iface->init = uca_pcowin_camera_initable_init;
}

static void
uca_pcowin_camera_class_init(UcaPcowinCameraClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = uca_pcowin_camera_set_property;
    gobject_class->get_property = uca_pcowin_camera_get_property;
    gobject_class->finalize = uca_pcowin_camera_finalize;

    UcaCameraClass *camera_class = UCA_CAMERA_CLASS(klass);
    camera_class->start_recording = uca_pcowin_camera_start_recording;
    camera_class->stop_recording = uca_pcowin_camera_stop_recording;
    camera_class->start_readout = uca_pcowin_camera_start_readout;
    camera_class->stop_readout = uca_pcowin_camera_stop_readout;
    camera_class->grab = uca_pcowin_camera_grab;
    camera_class->readout = uca_pcowin_camera_readout;
    camera_class->trigger = uca_pcowin_camera_trigger;
    
    for (guint i = 0; base_overrideables[i] != 0; i++)
        g_object_class_override_property(gobject_class, base_overrideables[i], uca_camera_props[base_overrideables[i]]);

    pco_properties[PROP_SENSOR_EXTENDED] =
        g_param_spec_boolean("sensor-extended",
            "Use extended sensor format",
            "Use extended sensor format",
            FALSE, G_PARAM_READWRITE);
            
    pco_properties[PROP_SENSOR_WIDTH_EXTENDED] =
        g_param_spec_uint("sensor-width-extended",
            "Width of extended sensor",
            "Width of the extended sensor in pixels",
            1, G_MAXUINT, 1,
            G_PARAM_READABLE);

    pco_properties[PROP_SENSOR_HEIGHT_EXTENDED] =
        g_param_spec_uint("sensor-height-extended",
            "Height of extended sensor",
            "Height of the extended sensor in pixels",
            1, G_MAXUINT, 1,
            G_PARAM_READABLE);
            
    pco_properties[PROP_SENSOR_TEMPERATURE] =
        g_param_spec_double("sensor-temperature",
            "Temperature of the sensor",
            "Temperature of the sensor in degree Celsius",
            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
            G_PARAM_READABLE);
            
     pco_properties[PROP_SENSOR_PIXELRATE] =
        g_param_spec_uint("sensor-pixelrate",
            "Pixel rate",
            "Pixel rate",
            1, G_MAXUINT, 1,
            G_PARAM_READWRITE);

    pco_properties[PROP_SENSOR_PIXELRATES] =
        g_param_spec_value_array("sensor-pixelrates",
            "Array of possible sensor pixel rates",
            "Array of possible sensor pixel rates",
            pco_properties[PROP_SENSOR_PIXELRATE],
            G_PARAM_READABLE);

    pco_properties[PROP_OFFSET_MODE] =
        g_param_spec_boolean("offset-mode",
            "Use offset mode",
            "Use offset mode",
            FALSE, G_PARAM_READWRITE);
            
    pco_properties[PROP_RECORD_MODE] =
        g_param_spec_enum("record-mode",
            "Record mode",
            "Record mode",
            UCA_TYPE_PCO_CAMERA_RECORD_MODE, UCA_PCO_CAMERA_RECORD_MODE_SEQUENCE,
            G_PARAM_READWRITE);

    pco_properties[PROP_STORAGE_MODE] =
        g_param_spec_enum("storage-mode",
            "Storage mode",
            "Storage mode",
            UCA_TYPE_PCO_CAMERA_STORAGE_MODE, UCA_PCO_CAMERA_STORAGE_MODE_FIFO_BUFFER,
            G_PARAM_READWRITE);

    pco_properties[PROP_ACQUIRE_MODE] =
        g_param_spec_enum("acquire-mode",
            "Acquire mode",
            "Acquire mode",
            UCA_TYPE_PCO_CAMERA_ACQUIRE_MODE, UCA_PCO_CAMERA_ACQUIRE_MODE_AUTO,
            G_PARAM_READWRITE);
    
    pco_properties[PROP_COOLING_POINT] =
        g_param_spec_int("cooling-point",
            "Cooling point of the camera",
            "Cooling point of the camera in degree celsius",
            0, 10, 5, G_PARAM_READWRITE);

    pco_properties[PROP_COOLING_POINT_MIN] =
        g_param_spec_int("cooling-point-min",
            "Minimum cooling point",
            "Minimum cooling point in degree celsius",
            G_MININT, G_MAXINT, 0, G_PARAM_READABLE);

    pco_properties[PROP_COOLING_POINT_MAX] =
        g_param_spec_int("cooling-point-max",
            "Maximum cooling point",
            "Maximum cooling point in degree celsius",
            G_MININT, G_MAXINT, 0, G_PARAM_READABLE);

    pco_properties[PROP_COOLING_POINT_DEFAULT] =
        g_param_spec_int("cooling-point-default",
            "Default cooling point",
            "Default cooling point in degree celsius",
            G_MININT, G_MAXINT, 0, G_PARAM_READABLE);

    pco_properties[PROP_VERSION] =
        g_param_spec_string("version",
            "Camera version",
            "Camera version given as `serial number, hardware major.minor, firmware major.minor'",
            NULL,
            G_PARAM_READABLE);
    
    for (guint id = N_BASE_PROPERTIES; id < N_PROPERTIES; id++)
        g_object_class_install_property(gobject_class, id, pco_properties[id]);
    
    g_type_class_add_private(klass, sizeof (UcaPcowinCameraPrivate));
}

static gint
setupsdk_and_opencamera (UcaPcowinCameraPrivate *priv, UcaPcowinCamera *camera)
{
    gint error;
    guint16 roi[4];
    
    priv->strGeneral.wSize = sizeof (priv->strGeneral);
    priv->strGeneral.strCamType.wSize = sizeof (priv->strGeneral.strCamType);
    priv->strCamType.wSize = sizeof (priv->strCamType);
    priv->strSensor.wSize = sizeof (priv->strSensor);
    priv->strSensor.strDescription.wSize = sizeof (priv->strSensor.strDescription);
    priv->strSensor.strDescription2.wSize = sizeof (priv->strSensor.strDescription2);
    priv->strDescription.wSize = sizeof (priv->strDescription);
    priv->strTiming.wSize = sizeof (priv->strTiming);
    priv->strStorage.wSize = sizeof (priv->strStorage);
    priv->strRecording.wSize = sizeof (priv->strRecording);
    
    // 0 - Success, ~0 - Error or warning
    error = PCO_OpenCamera(&priv->pcoHandle, 0);
    if(0 == error)
    {
        PCO_GetGeneral (priv->pcoHandle, &priv->strGeneral);
        PCO_GetCameraType (priv->pcoHandle, &priv->strCamType);
        PCO_GetSensorStruct (priv->pcoHandle, &priv->strSensor);
        PCO_GetCameraDescription (priv->pcoHandle, &priv->strDescription);
        PCO_GetTimingStruct (priv->pcoHandle, &priv->strTiming);
        PCO_GetRecordingStruct (priv->pcoHandle, &priv->strRecording);

        // There variables are used in 4 cases of get_parameters function
        PCO_GetROI(priv->pcoHandle, &roi[0], &roi[1], &roi[2], &roi[3]);
        priv->roi_x = roi[0] - 1;
        priv->roi_y = roi[1] - 1;
        priv->roi_width = roi[2] - roi[0] + 1;
        priv->roi_height = roi[3] - roi[1] + 1;

        
    }
    return error;
}

static void
uca_pcowin_camera_init(UcaPcowinCamera *self)
{
    UcaPcowinCameraPrivate *priv; 
    UcaPcowinCamera *camera;
    gint error;
    
    priv = UCA_PCOWIN_CAMERA_GET_PRIVATE(self);
    priv->construct_error = NULL;
    
    error = setupsdk_and_opencamera (priv,self);
    if(error)
    {
        g_set_error (&priv->construct_error,
                     UCA_PCOWIN_CAMERA_ERROR, UCA_PCOWIN_CAMERA_ERROR_SDK_INIT,
                     "Initialization of SDK Failed. Here's error code 0x%X for enquiring minds", error);
    }
    
    camera = UCA_CAMERA (self);
    uca_camera_register_unit (camera, "sensor-width-extended", UCA_UNIT_PIXEL);
    uca_camera_register_unit (camera, "sensor-height-extended", UCA_UNIT_PIXEL);
    uca_camera_register_unit (camera, "sensor-temperature", UCA_UNIT_DEGREE_CELSIUS);
    uca_camera_register_unit (camera, "cooling-point", UCA_UNIT_DEGREE_CELSIUS);
    uca_camera_register_unit (camera, "cooling-point-min", UCA_UNIT_DEGREE_CELSIUS);
    uca_camera_register_unit (camera, "cooling-point-max", UCA_UNIT_DEGREE_CELSIUS);
    uca_camera_register_unit (camera, "cooling-point-default", UCA_UNIT_DEGREE_CELSIUS);
    
    uca_camera_set_writable (camera, "exposure-time", TRUE);
    uca_camera_set_writable (camera, "frames-per-second", TRUE);
}

G_MODULE_EXPORT GType
camera_plugin_get_type (void)
{
    return UCA_TYPE_PCOWIN_CAMERA;
}