/**
Notes:
    PCO SDK uses windows specific data types in its header files and documentation.
    This program has glib data types. For ex. for a pco library function that requires WORD, 
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
#define SENSORFORMAT_STANDARD           0x0000
#define SENSORFORMAT_EXTENDED           0x0001
#define ERROR_TEXT_BUFFER_SIZE          500
#define TIMESTAMP_MODE_OFF              0x0000
#define TIMESTAMP_MODE_BINARY           0x0001
#define TIMESTAMP_MODE_BINARYANDASCII   0x0002
#define TIMESTAMP_MODE_ASCII            0x0003  // Not available in dimax

#define UCA_PCOWIN_CAMERA_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), UCA_TYPE_PCOWIN_CAMERA, UcaPcowinCameraPrivate))

#define CHECK_FOR_PCO_SDK_ERROR(err)                                                                \
    if (err != 0) {                                                                                 \
        PCO_GetErrorText(err, error_text, ERROR_TEXT_BUFFER_SIZE);                                  \
        g_set_error (error, UCA_PCOWIN_CAMERA_ERROR,                                                \
                     UCA_PCOWIN_CAMERA_ERROR_SDKERROR,                                              \
                     "Error occured in PCO SDK. Here's error code 0x%X for enquiring minds.\nSDK Error Text: %s",err,error_text);   \
        return;                                                                                     \
    }

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
    //PROP_OFFSET_MODE,
    PROP_RECORD_MODE,
    PROP_STORAGE_MODE,
    PROP_ACQUIRE_MODE,
    //PROP_FAST_SCAN,
    //PROP_COOLING_POINT,
    PROP_COOLING_POINT_MIN,
    PROP_COOLING_POINT_MAX,
    PROP_COOLING_POINT_DEFAULT,
    //PROP_NOISE_FILTER,
    PROP_TIMESTAMP_MODE,
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

static char error_text[ERROR_TEXT_BUFFER_SIZE];

struct _UcaPcowinCameraPrivate {
    
    HANDLE pcoHandle;
    HANDLE handleEvent;
    
    GError *construct_error;
    
    PCO_General strGeneral;
    PCO_CameraType strCamType;
    PCO_Sensor strSensor;
    PCO_Description strDescription;
    PCO_Timing strTiming;
    PCO_Storage strStorage;
    PCO_Recording strRecording;

    guint16 roi_x, roi_y;
    guint16 roi_width, roi_height;
    guint16 roi_horizontal_steps, roi_vertical_steps;
    guint16 width, height, width_ex, height_ex;
    guint16 x_act, y_act;
    guint16 horizontal_binning, vertical_binning;
    GValueArray *possible_pixelrates;
    guint32 pixel_rate;

    // With buff number = -1 and bufferPointer set to 0. SDK allocated a buffer for us
    gint16 buffer_number; 
    guint16 *buffer_pointer;
    guint32 buffer_size;
    guint16 active_ram_segment;
    guint32 numberof_recorded_images, camram_max_images, current_image;

    UcaCameraTriggerSource trigger_source;
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

static void
uca_pcowin_camera_start_recording(UcaCamera *camera, GError **error)
{
    UcaPcowinCameraPrivate *priv;

    guint16 binned_width, binned_height;
    gboolean use_extended_sensor_format;
    gboolean transfer_async;
    guint err;
    int library_errors;

    g_return_if_fail (UCA_IS_PCOWIN_CAMERA (camera));

    priv = UCA_PCOWIN_CAMERA_GET_PRIVATE (camera);

    g_object_get (camera,
                  "trigger-source", &priv->trigger_source,
                  "sensor-extended", &use_extended_sensor_format,
                  "transfer-asynchronously", &transfer_async,
                  NULL);

    PCO_ClearRamSegment(priv->pcoHandle);

    if(use_extended_sensor_format)
    {
        binned_width = priv->width_ex;
        binned_height = priv->height_ex;
    }
    else
    {
        binned_width = priv->width;
        binned_height = priv->height;
    }

    // For priliminary tests. @ToDo Clenup
    priv->horizontal_binning = priv->vertical_binning = 1;

    // The following peice of code make sures that the ROI defined is not beyond the limits of available area binning
    binned_width /= priv->horizontal_binning;
    binned_height /= priv->vertical_binning;

    if ((priv->roi_x + priv->roi_width > binned_width) || (priv->roi_y + priv->roi_height > binned_height)) {
        g_set_error (error, UCA_PCOWIN_CAMERA_ERROR, UCA_PCOWIN_CAMERA_ERROR_UNSUPPORTED,
                     "ROI of size %ix%i @ (%i, %i) is outside of (binned) sensor size %ix%i\n",
                     priv->roi_width, priv->roi_height, priv->roi_x, priv->roi_y, binned_width, binned_height);
        return;
    }

    library_errors = PCO_SetBinning(priv->pcoHandle, priv->horizontal_binning, priv->vertical_binning);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);
    
    guint16 roi[4] = { priv->roi_x + 1, priv->roi_y + 1, priv->roi_x + priv->roi_width, priv->roi_y + priv->roi_height };
    library_errors = PCO_SetROI(priv->pcoHandle, roi[0], roi[1], roi[2], roi[3]);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);

    library_errors = PCO_ArmCamera(priv->pcoHandle);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);

    // Diagnostics
    guint32 status, warnus, errnus;
    library_errors = PCO_GetCameraHealthStatus(priv->pcoHandle, &warnus, &errnus, &status);
    //g_printf("0x%X \t 0x%X \t 0x%X \t",warnus,errnus,status);

    // Get actual armed (also locked and loaded, ready to fire the hell out) image sizes from camera. This data is used to allocate buffer
    guint16 x_act, y_act, x_max, y_max;
    library_errors = PCO_GetSizes(priv->pcoHandle, &x_act, &y_act, &x_max, &y_max);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);
    // @ToDo. Implement checks to verify if the sizes received from camera are same as ROI
    priv->x_act = x_act;
    priv->y_act = y_act;

    // Allocation of buffer. Driver allocates a number if buffer number is set to -1
    priv->buffer_number = -1; 
    priv->buffer_size = x_act * y_act * 2;
    priv->buffer_pointer = NULL;
    priv->handleEvent = NULL;

    library_errors = PCO_AllocateBuffer(priv->pcoHandle, &priv->buffer_number, priv->buffer_size, &priv->buffer_pointer, &priv->handleEvent);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);

    library_errors = PCO_CamLinkSetImageParameters(priv->pcoHandle, x_act, y_act);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);

    library_errors = PCO_SetRecordingState(priv->pcoHandle, 0x0001);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);


}

static void
uca_pcowin_camera_stop_recording(UcaCamera *camera, GError **error)
{
    UcaPcowinCameraPrivate *priv;
    int library_errors;

    g_return_if_fail(UCA_IS_PCOWIN_CAMERA(camera));

    priv = UCA_PCOWIN_CAMERA_GET_PRIVATE (camera);

    library_errors = PCO_CancelImages(priv->pcoHandle);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);

    library_errors = PCO_SetRecordingState(priv->pcoHandle, 0x0000);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);

    library_errors = PCO_GetPixelRate(priv->pcoHandle, &priv->pixel_rate);

    /*guint32 status, warnus, errnus;
    library_errors = PCO_GetCameraHealthStatus(priv->pcoHandle, &warnus, &errnus, &status);*/
}

static void
uca_pcowin_camera_start_readout(UcaCamera *camera, GError **error)
{
    UcaPcowinCameraPrivate *priv;
    g_return_if_fail(UCA_IS_PCOWIN_CAMERA(camera));
    priv=UCA_PCOWIN_CAMERA_GET_PRIVATE(camera);

    PCO_GetNumberOfImagesInSegment(priv->pcoHandle, priv->active_ram_segment, &priv->numberof_recorded_images, &priv->camram_max_images);
    priv->current_image = 1;
}

static void
uca_pcowin_camera_stop_readout(UcaCamera *camera, GError **error)
{
}

static void
uca_pcowin_camera_trigger (UcaCamera *camera, GError **error)
{
    int library_errors;
    UcaPcowinCameraPrivate *priv;
    guint16 trigger_state;
    guint16 is_camera_busy;

    g_return_if_fail(UCA_IS_PCOWIN_CAMERA(camera));

    priv=UCA_PCOWIN_CAMERA_GET_PRIVATE(camera);

    /*If a trigger fails it will not trigger future exposures. 
    Therefore calling forcetrigger is prevented if camera is busy*/
    library_errors = PCO_GetCameraBusyStatus(priv->pcoHandle, &is_camera_busy);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);
    if(is_camera_busy)
    {
        g_set_error(error, UCA_PCOWIN_CAMERA_ERROR, UCA_PCOWIN_CAMERA_ERROR_GENERAL, "Software trigger was prevented because camera is busy");
    }
    else
    {
        library_errors = PCO_ForceTrigger(priv->pcoHandle, &trigger_state);
        CHECK_FOR_PCO_SDK_ERROR(library_errors);
    }
}

static gboolean
uca_pcowin_camera_grab (UcaCamera *camera, gpointer data, GError **error)
{
    int library_errors;
    UcaPcowinCameraPrivate *priv;
    gboolean is_readout;
    guint32 image_index_to_transfer = 0;

    g_return_if_fail(UCA_IS_PCOWIN_CAMERA(camera));

    priv=UCA_PCOWIN_CAMERA_GET_PRIVATE(camera);

    /*
    This function acts similar to AddBufferEx. i.e to view images while recording is enabled. 
    Except event handeling is not required.
    Fn returns only when the image is transferred to memory (5 sec timeout)
    0,0 transfers most recent image. 1,1 could be used to transfer the first captured image ?
    firstImage and LastImage of the segment can be used to bulk read images from camRAM. - This is not possible for dimax. Though mentioned in the SDK, it doesn't work as advertised
    Note. Make sure buffersize of appropriate buffernumber is sufficient enough when transferring multiple images.
    */
    // "is_readout" is set in uca_camera_start_readout and unset in uca_camera_stop_readout.
    g_object_get (G_OBJECT (camera), "is-readout", &is_readout, NULL);
    if(is_readout)
    {
        /**
            Error set to convey that the readout index has reached last available frame on camRAM
        **/
        if(priv->current_image > priv->numberof_recorded_images)
        {
            // Should this is be used ?
            g_set_error (error, UCA_CAMERA_ERROR,
                     UCA_CAMERA_ERROR_END_OF_STREAM,
                     "End of camRAM readout");
            return FALSE;
        }

        image_index_to_transfer = priv->current_image;
        priv->current_image ++;
    }
    library_errors = PCO_GetImageEx(priv->pcoHandle, priv->active_ram_segment, image_index_to_transfer, image_index_to_transfer, priv->buffer_number, priv->x_act, priv->y_act, priv->pixel_rate);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);
    memcpy((gchar *) data, (gchar *) priv->buffer_pointer, priv->buffer_size);

    return TRUE;
}

static gboolean
uca_pcowin_camera_readout (UcaCamera *camera, gpointer data, guint index, GError **error)
{
    UcaPcowinCameraPrivate *priv;
    int library_errors;

    g_return_if_fail(UCA_IS_PCOWIN_CAMERA(camera));

    priv = UCA_PCOWIN_CAMERA_GET_PRIVATE(camera);

    library_errors = PCO_GetImageEx(priv->pcoHandle, priv->active_ram_segment, index, index, priv->buffer_number, priv->x_act, priv->y_act, priv->pixel_rate);
    CHECK_FOR_PCO_SDK_ERROR(library_errors);

    memcpy((gchar *) data, (gchar *) priv->buffer_pointer, priv->buffer_size);

    return TRUE;
}

static void
uca_pcowin_camera_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    UcaPcowinCameraPrivate *priv = UCA_PCOWIN_CAMERA_GET_PRIVATE (object);
    int library_errors;

    if (uca_camera_is_recording (UCA_CAMERA (object)) && !uca_camera_is_writable_during_acquisition (UCA_CAMERA (object), pspec->name)) {
        g_warning ("Property '%s' can not be changed during acquisition", pspec->name);
        return;
    }

    switch(property_id) {
        case PROP_SENSOR_EXTENDED:
            {
                guint16 format = g_value_get_boolean (value) ? SENSORFORMAT_EXTENDED : SENSORFORMAT_STANDARD;
                library_errors = PCO_SetSensorFormat(priv->pcoHandle, format);
            }
            break;
        case PROP_ROI_X:
            priv->roi_x = g_value_get_uint(value);
            break;
        case PROP_ROI_Y:
            priv->roi_y = g_value_get_uint(value);
            break;
        case PROP_ROI_WIDTH:
            priv->roi_width = g_value_get_uint(value);
            break;
        case PROP_ROI_HEIGHT:
            priv->roi_height = g_value_get_uint(value);
            break;
        case PROP_SENSOR_HORIZONTAL_BINNING:
            {
                if(g_value_get_uint(value) <= priv->strDescription.wMaxBinHorzDESC)
                    priv->horizontal_binning = g_value_get_uint(value);    
                else
                    g_warning("Horizontal binning value exceeds maximum horizontal binning of camera");
            }
            break;
        case PROP_SENSOR_VERTICAL_BINNING:
            {
                if(g_value_get_uint(value) <= priv->strDescription.wMaxBinVertDESC)
                    priv->vertical_binning = g_value_get_uint(value);
                else
                    g_warning("Vertical binning value exceeds maximum vertical binning of camera");
            }
            break;
        case PROP_EXPOSURE_TIME:
            {
                guint16 framerate_status;
                guint16 mode_exposure_has_priority = 0x0002;
                guint32 framerate, framerate_exposure; //Exposure time is in ns
                library_errors = PCO_GetFrameRate(priv->pcoHandle, &framerate_status, &framerate, &framerate_exposure);
                framerate_exposure = g_value_get_double(value) * 1000 * 1000 * 1000;
                library_errors = PCO_SetFrameRate(priv->pcoHandle, &framerate_status, mode_exposure_has_priority, &framerate, &framerate_exposure);
            }
            break;
        case PROP_FRAMES_PER_SECOND:
            {
                // Works in dimax and edge only. @ToDo Write implementation for other cameras
                guint16 framerate_status;
                guint16 mode_framerate_has_priority = 0x0001;
                guint32 framerate, framerate_exposure; //Exposure time is in ns
                library_errors = PCO_GetFrameRate(priv->pcoHandle, &framerate_status, &framerate, &framerate_exposure);
                framerate = g_value_get_double(value) * 1000;
                library_errors = PCO_SetFrameRate(priv->pcoHandle, &framerate_status, mode_framerate_has_priority, &framerate, &framerate_exposure);
                // @ToDo. framerate_status variables hold information if the desired framerate was trimmed because of other settings
            }
            break;
        case PROP_SENSOR_PIXELRATE:
            {
                guint32 pixelrate; // user defined pixelrate in Hz
                guint32 pixelrate_to_set = 0;
                pixelrate = g_value_get_uint(value);
                for(int i=0; i< priv->possible_pixelrates->n_values; i++)
                {
                    if(g_value_get_uint(g_value_array_get_nth(priv->possible_pixelrates,i)) == pixelrate)
                    {
                        pixelrate_to_set = pixelrate;
                        break;
                    }
                }
                if(pixelrate_to_set)
                    library_errors = PCO_SetPixelRate(priv->pcoHandle, pixelrate_to_set);
                else
                    g_warning("Pixelrate is not set. %d Hz is not in the range of possible pixelrates. Check \'sensor-pixelrates\' property",pixelrate);
            }
            break;
        /*case PROP_OFFSET_MODE:
            {
                // PCO_SetOffsetMode is available only for pco.1400, pco.pixelfly.usb, pco.1300. @ToDo implement proper checks
                if(CAMERATYPE_PCO_DIMAX_STD != priv->strCamType.wCamType)
                    library_errors = PCO_SetOffsetMode(priv->pcoHandle, g_value_get_boolean(value) ? 1 : 0);    
            }
            break;
        case PROP_COOLING_POINT:
            {
                // PCO_SetCoolingSetpointTemperature is only available for 1300,1600,2000,4000. @ToDo implement proper checks
                gint16 temperature;
                if(CAMERATYPE_PCO_DIMAX_STD != priv->strCamType.wCamType)
                {
                    temperature = (gint16)g_value_get_int(value);
                    library_errors = PCO_SetCoolingSetpointTemperature(priv->pcoHandle, temperature);
                }
            }
            break;*/
        case PROP_RECORD_MODE:
            {
                UcaPcoCameraRecordMode subMode = (UcaPcoCameraRecordMode) g_value_get_enum(value);

                switch(subMode)
                {
                    case UCA_PCO_CAMERA_RECORD_MODE_SEQUENCE:
                        library_errors = PCO_SetRecorderSubmode(priv->pcoHandle,RECORDER_SUBMODE_SEQUENCE);
                        break;
                    case UCA_PCO_CAMERA_RECORD_MODE_RING_BUFFER:
                        library_errors = PCO_SetRecorderSubmode(priv->pcoHandle, RECORDER_SUBMODE_RINGBUFFER);
                        break;
                }
            }
            break;
        case PROP_STORAGE_MODE:
            {
                UcaPcoCameraStorageMode storageMode = (UcaPcoCameraStorageMode) g_value_get_enum(value);

                switch (storageMode)
                {
                    case UCA_PCO_CAMERA_STORAGE_MODE_RECORDER:
                        library_errors = PCO_SetStorageMode (priv->pcoHandle, STORAGE_MODE_RECORDER);
                        break;
                    case UCA_PCO_CAMERA_STORAGE_MODE_FIFO_BUFFER:
                        library_errors = PCO_SetStorageMode (priv->pcoHandle, STORAGE_MODE_FIFO_BUFFER);
                        break;
                }
            }
            break;
        case PROP_ACQUIRE_MODE:
            {
                UcaPcoCameraAcquireMode acqMode = (UcaPcoCameraAcquireMode) g_value_get_enum(value);

                switch (acqMode)
                {
                    case UCA_PCO_CAMERA_ACQUIRE_MODE_AUTO:
                        library_errors = PCO_SetAcquireMode (priv->pcoHandle, ACQUIRE_MODE_AUTO);
                        break;
                    case UCA_PCO_CAMERA_ACQUIRE_MODE_EXTERNAL:
                        library_errors = PCO_SetAcquireMode (priv->pcoHandle, ACQUIRE_MODE_EXTERNAL);
                        break;
                }
            }
            break;
        case PROP_TRIGGER_SOURCE:
            {
                UcaCameraTriggerSource triggerMode = (UcaCameraTriggerSource) g_value_get_enum(value);

                switch (triggerMode) {
                    case UCA_CAMERA_TRIGGER_SOURCE_AUTO:
                        library_errors = PCO_SetTriggerMode(priv->pcoHandle, TRIGGER_MODE_AUTOTRIGGER);
                        break;
                    case UCA_CAMERA_TRIGGER_SOURCE_SOFTWARE:
                        library_errors = PCO_SetTriggerMode(priv->pcoHandle, TRIGGER_MODE_SOFTWARETRIGGER);
                        break;
                    case UCA_CAMERA_TRIGGER_SOURCE_EXTERNAL:
                        library_errors = PCO_SetTriggerMode(priv->pcoHandle, TRIGGER_MODE_EXTERNALTRIGGER);
                        break;
                    default:
                        g_warning("Trigger mode provided by camera cannot be handled");
                }
            }
            break;
        case PROP_TIMESTAMP_MODE:
            {
                UcaPcoCameraTimestamp timestamp_mode = g_value_get_enum(value);
                switch(timestamp_mode) {
                    case UCA_PCO_CAMERA_TIMESTAMP_NONE:
                        PCO_SetTimestampMode (priv->pcoHandle, TIMESTAMP_MODE_OFF);
                        break;
                    case UCA_PCO_CAMERA_TIMESTAMP_BINARY:
                        PCO_SetTimestampMode (priv->pcoHandle, TIMESTAMP_MODE_BINARY);
                        break;
                    case UCA_PCO_CAMERA_TIMESTAMP_BINARYANDASCII:
                        PCO_SetTimestampMode (priv->pcoHandle, TIMESTAMP_MODE_BINARYANDASCII);
                        break;
                    case UCA_PCO_CAMERA_TIMESTAMP_ASCII:
                        PCO_SetTimestampMode (priv->pcoHandle, TIMESTAMP_MODE_ASCII);
                        break;
                }
            }
            break;
        default:
            g_warning("Undefined Property");
    }

    if(library_errors)
    {
        PCO_GetErrorText(library_errors,error_text, ERROR_TEXT_BUFFER_SIZE);
        g_warning ("Failed to set property %s. Here's error code 0x%X for enquiring minds.\nSDK Error Text: %s", pco_properties[property_id]->name, library_errors, error_text);
    }
}

static void
uca_pcowin_camera_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    UcaPcowinCameraPrivate *priv;
    int library_errors;
    
    priv = UCA_PCOWIN_CAMERA_GET_PRIVATE(object);
    
    switch (property_id) {
        case PROP_SENSOR_EXTENDED:
        {
            guint16 format;
            library_errors = PCO_GetSensorFormat(priv->pcoHandle,&format);
            g_value_set_boolean(value, (format==SENSORFORMAT_EXTENDED));
        }
            break;
        case PROP_SENSOR_WIDTH_EXTENDED:
            g_value_set_uint(value, priv->width_ex < priv->width ? priv->width : priv->width_ex);
            break;
        case PROP_SENSOR_HEIGHT_EXTENDED:
            g_value_set_uint(value, priv->height_ex < priv->height ? priv->height : priv->height_ex);
            break;
        case PROP_SENSOR_TEMPERATURE:
        {
            short ccdTemp, camTemp, powTemp;
            library_errors = PCO_GetTemperature(priv->pcoHandle,&ccdTemp, &camTemp, &powTemp);
            g_value_set_double(value,ccdTemp/10);
        }
            break;
        case PROP_SENSOR_PIXELRATES:
            g_value_set_boxed(value, priv->possible_pixelrates);
            break;
        case PROP_SENSOR_PIXELRATE:
        {
            guint32 pixelrate; // pixelrate in Hz
            library_errors = PCO_GetPixelRate(priv->pcoHandle, &pixelrate);
            g_value_set_uint(value, pixelrate);
        }
            break;
        /*case PROP_OFFSET_MODE:
            {
                guint16 offsetRegulation = FALSE;
                if(CAMERATYPE_PCO_DIMAX_STD != priv->strCamType.wCamType || CAMERATYPE_PCO_EDGE != priv->strCamType.wCamType)
                {
                    library_errors = PCO_GetOffsetMode(priv->pcoHandle, &offsetRegulation);    
                }
                g_value_set_boolean(value, offsetRegulation ? TRUE: FALSE);
            }
            break;*/
        case PROP_RECORD_MODE:
            {
                // Only available if the storage mode is set to recorder
                guint16 recorderSubmode;
                library_errors = PCO_GetRecorderSubmode(priv->pcoHandle, &recorderSubmode);

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
                library_errors = PCO_GetStorageMode(priv->pcoHandle, &storageMode);

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
                library_errors = PCO_GetAcquireMode(priv->pcoHandle, &acqMode);

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
        /*case PROP_COOLING_POINT:
            {
                gint16 coolingSetPoint = 0;
                if(CAMERATYPE_PCO_DIMAX_STD != priv->strCamType.wCamType)
                    library_errors = PCO_GetCoolingSetpointTemperature(priv->pcoHandle, &coolingSetPoint);
                g_value_set_int(value, coolingSetPoint);
            }
            break;*/
        case PROP_COOLING_POINT_MIN:
            g_value_set_int(value,priv->strDescription.sMinCoolSetDESC);
            break;
        case PROP_COOLING_POINT_MAX:
            g_value_set_int(value,priv->strDescription.sMaxCoolSetDESC);
            break;
        case PROP_COOLING_POINT_DEFAULT:
            g_value_set_int(value,priv->strDescription.sDefaultCoolSetDESC);
            break;
        case PROP_TIMESTAMP_MODE:
            {
                guint16 timestamp_mode;
                library_errors = PCO_GetTimestampMode (priv->pcoHandle, &timestamp_mode);
                switch(timestamp_mode) {
                    case TIMESTAMP_MODE_OFF:
                        g_value_set_enum (value, UCA_PCO_CAMERA_TIMESTAMP_NONE);
                        break;
                    case TIMESTAMP_MODE_BINARY:
                        g_value_set_enum (value, UCA_PCO_CAMERA_TIMESTAMP_BINARY);
                        break;
                    case TIMESTAMP_MODE_BINARYANDASCII:
                        g_value_set_enum (value, UCA_PCO_CAMERA_TIMESTAMP_BINARYANDASCII);
                        break;
                    case TIMESTAMP_MODE_ASCII:
                        g_value_set_enum (value, UCA_PCO_CAMERA_TIMESTAMP_ASCII);
                        break;
                }
            }
            break;
        // Generic UCA Properties
        case PROP_SENSOR_WIDTH:
            g_value_set_uint(value, priv->width);
            break;
        case PROP_SENSOR_HEIGHT:
            g_value_set_uint(value, priv->height);
            break;
        case PROP_SENSOR_PIXEL_WIDTH:
            switch (priv->strCamType.wCamType) {
                case CAMERATYPE_PCO_EDGE:
                    g_value_set_double (value, 0.0000065);
                    break;
                case CAMERATYPE_PCO_EDGE_GL:
                    g_value_set_double (value, 0.0000065);
                    break;
                case CAMERATYPE_PCO_DIMAX_STD:
                    g_value_set_double (value, 0.0000110);
                    break;
                case CAMERATYPE_PCO4000:
                    g_value_set_double (value, 0.0000090);
                    break;
            }
            break;
        case PROP_SENSOR_PIXEL_HEIGHT:
            switch (priv->strCamType.wCamType) {
                case CAMERATYPE_PCO_EDGE:
                    g_value_set_double (value, 0.0000065);
                    break;
                case CAMERATYPE_PCO_EDGE_GL:
                    g_value_set_double (value, 0.0000065);
                    break;
                case CAMERATYPE_PCO_DIMAX_STD:
                    g_value_set_double (value, 0.0000110);
                    break;
                case CAMERATYPE_PCO4000:
                    g_value_set_double (value, 0.0000090);
                    break;
            }
            break;
        case PROP_SENSOR_HORIZONTAL_BINNING:
            {
                PCO_GetBinning(priv->pcoHandle, &priv->horizontal_binning, &priv->vertical_binning);
                g_value_set_uint(value, priv->horizontal_binning);
            }
            break;
        case PROP_SENSOR_VERTICAL_BINNING:
            {
                PCO_GetBinning(priv->pcoHandle, &priv->horizontal_binning, &priv->vertical_binning);
                g_value_set_uint(value, priv->vertical_binning);
            }
            break;
        case PROP_SENSOR_BITDEPTH:
            switch (priv->strCamType.wCamType) {
                case CAMERATYPE_PCO4000:
                    g_value_set_uint(value, 14);
                    break;
                case CAMERATYPE_PCO_EDGE:
                    g_value_set_uint(value, 16);
                    break;
                case CAMERATYPE_PCO_EDGE_GL:
                    g_value_set_uint(value, 16);
                    break;
                case CAMERATYPE_PCO_DIMAX_STD:
                    g_value_set_uint(value, 16); // Why do we get image with bit depth of 16 bit ?
                    break;
            }
            break;
        case PROP_EXPOSURE_TIME:
        {
            // Works for dimax and edge only
            guint16 framerate_status;
            guint32 framerate, framerate_exposure;
            library_errors = PCO_GetFrameRate(priv->pcoHandle, &framerate_status, &framerate, &framerate_exposure);
            g_value_set_double(value, framerate_exposure / 1000. / 1000. / 1000.);
        }
            break;
        case PROP_FRAMES_PER_SECOND:
        {
            // Works for dimax and edge only
            guint16 framerate_status;
            guint32 framerate, framerate_exposure;
            library_errors = PCO_GetFrameRate(priv->pcoHandle, &framerate_status, &framerate, &framerate_exposure);
            g_value_set_double(value, framerate / 1000.); //mHz to Hz
        }
            break;
        case PROP_HAS_STREAMING:
            g_value_set_boolean(value, TRUE); 
            break;

        case PROP_HAS_CAMRAM_RECORDING:
            // @ToDo. PCI Dimax has camRAM, but few PCO cameras do not. Replace this for expanding uca-pco-win plugin for other cameras
            g_value_set_boolean(value, TRUE);
            break;

        case PROP_RECORDED_FRAMES:
        {
            // This number is dynamic if the camera is running in recorder mode or in FIFO buffer mode. Result is accurate when recording is stopped
            guint32 valid_images, max_images;
            library_errors = PCO_GetNumberOfImagesInSegment(priv->pcoHandle, priv->active_ram_segment, &valid_images, &max_images);
            g_value_set_uint(value, valid_images);
        }
            break;      
        case PROP_TRIGGER_SOURCE:
        {
            guint16 trigger_mode;
            library_errors = PCO_GetTriggerMode(priv->pcoHandle, &trigger_mode);

            switch (trigger_mode) {
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
            g_value_set_uint(value, priv->roi_horizontal_steps);
            break;
        case PROP_ROI_HEIGHT_MULTIPLIER:
            g_value_set_uint(value, priv->roi_vertical_steps);
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
                guint16 recording_state;
                library_errors = PCO_GetRecordingState(priv->pcoHandle, &recording_state);
                g_value_set_boolean(value, recording_state ? TRUE: FALSE);
            }
            break;
        default:
            g_warning("Undefined Property");
    }

    if(library_errors)
    {
        PCO_GetErrorText(library_errors, error_text, ERROR_TEXT_BUFFER_SIZE);
        g_warning ("Failed to set property %s. Here's error code 0x%X for enquiring minds.\nSDK Error Text: %s", pco_properties[property_id]->name, library_errors, error_text);
    }
}

static void
uca_pcowin_camera_finalize(GObject *object)
{
    UcaPcowinCameraPrivate *priv = UCA_PCOWIN_CAMERA_GET_PRIVATE(object);
    
    // Clearing all allocated memories
    if(priv->possible_pixelrates)
        g_value_array_free(priv->possible_pixelrates);
    g_clear_error (&priv->construct_error);

    /** 
        Buffers are allocated during start_recording. So, should be freed at the end
        @ToDo. If there are any previous buffers (when the camera acquires multiple times)
        Then, only the last buffer is cleared. Though the SDK handles freeing of buffer. It logs this behaviour as an error.
    **/
    PCO_FreeBuffer (priv->pcoHandle, priv->buffer_number);

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

    /*pco_properties[PROP_OFFSET_MODE] =
        g_param_spec_boolean("offset-mode",
            "Use offset mode",
            "Use offset mode",
            FALSE, G_PARAM_READWRITE);*/
            
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
    
    /*pco_properties[PROP_COOLING_POINT] =
        g_param_spec_int("cooling-point",
            "Cooling point of the camera",
            "Cooling point of the camera in degree celsius",
            0, 10, 5, G_PARAM_READWRITE);*/

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

    pco_properties[PROP_TIMESTAMP_MODE] =
        g_param_spec_enum("timestamp-mode",
            "Timestamp mode",
            "Timestamp mode",
            UCA_TYPE_PCO_CAMERA_TIMESTAMP, UCA_PCO_CAMERA_TIMESTAMP_BINARYANDASCII,
            G_PARAM_READWRITE);
    
    for (guint id = N_BASE_PROPERTIES; id < N_PROPERTIES; id++)
        g_object_class_install_property(gobject_class, id, pco_properties[id]);
    
    g_type_class_add_private(klass, sizeof (UcaPcowinCameraPrivate));
}

static void
property_override_default_guint_value (GObjectClass *oclass, const gchar *property_name, guint new_default)
{
    GParamSpecUInt *pspec = G_PARAM_SPEC_UINT (g_object_class_find_property (oclass, property_name));

    if (pspec != NULL)
        pspec->default_value = new_default;
    else
        g_warning ("pspec for %s not found\n", property_name);
}

static void
set_default_properties(UcaPcowinCamera *camera)
{
    UcaPcowinCameraPrivate *priv;
    GObjectClass *oclass;

    priv = UCA_PCOWIN_CAMERA_GET_PRIVATE(camera);
    oclass = G_OBJECT_CLASS (UCA_PCOWIN_CAMERA_GET_CLASS(camera));

    // Get possible pixel rates from PCO Description and set default pixelrate
    GValue temporary = {0};
    g_value_init(&temporary, G_TYPE_UINT);
    priv->possible_pixelrates = g_value_array_new(4);
    for(int i=0; i< 4; i++) // SDK provides 4 possible pixelrate
    {
        g_value_set_uint(&temporary, (guint) priv->strDescription.dwPixelRateDESC[i]);
        g_value_array_append(priv->possible_pixelrates, &temporary);
    }
    property_override_default_guint_value (oclass, "sensor-pixelrate", priv->strDescription.dwPixelRateDESC[0]);

    property_override_default_guint_value (oclass, "roi-width", priv->width);
    property_override_default_guint_value (oclass, "roi-height", priv->height);
    //Temperature & ADCS
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
        //PCO_ResetSettingsToDefault(priv->pcoHandle);
        // @ToDo. Do error checking for the following functions. Remove unnecessary calls
        PCO_GetGeneral (priv->pcoHandle, &priv->strGeneral);
        PCO_GetCameraType (priv->pcoHandle, &priv->strCamType);
        PCO_GetSensorStruct (priv->pcoHandle, &priv->strSensor);
        PCO_GetCameraDescription (priv->pcoHandle, &priv->strDescription);
        PCO_GetTimingStruct (priv->pcoHandle, &priv->strTiming);
        PCO_GetRecordingStruct (priv->pcoHandle, &priv->strRecording);

        // UcaCamera variables are filled with initial values from camera description or sensor
        PCO_GetROI(priv->pcoHandle, &roi[0], &roi[1], &roi[2], &roi[3]);
        priv->roi_x = roi[0] - 1;
        priv->roi_y = roi[1] - 1;
        priv->roi_width = roi[2] - roi[0] + 1;
        priv->roi_height = roi[3] - roi[1] + 1;
        priv->roi_horizontal_steps= priv->strDescription.wRoiHorStepsDESC;
        priv->roi_vertical_steps= priv->strDescription.wRoiVertStepsDESC;

        priv->width = priv->strDescription.wMaxHorzResStdDESC;
        priv->height = priv->strDescription.wMaxVertResStdDESC;
        priv->width_ex = priv->strDescription.wMaxHorzResExtDESC;
        priv->height_ex = priv->strDescription.wMaxVertResExtDESC;

        PCO_GetBinning(priv->pcoHandle, &priv->horizontal_binning, &priv->vertical_binning);

        PCO_GetActiveRamSegment(priv->pcoHandle, &priv->active_ram_segment);
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
        PCO_GetErrorText(error, error_text, ERROR_TEXT_BUFFER_SIZE);
        g_set_error (&priv->construct_error,
                     UCA_PCOWIN_CAMERA_ERROR, UCA_PCOWIN_CAMERA_ERROR_SDK_INIT,
                     "Initialization of SDK Failed. Here's error code 0x%X for enquiring minds.\nSDK Error Text: %s", error, error_text);
    }

    set_default_properties(self);
    
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