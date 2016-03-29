/*
Copyright (C) 2016  Sai Sasidhar Maddali <sai.sasidhar92@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __UCA_PCOWIN_CAMERA_H
#define __UCA_PCOWIN_CAMERA_H

#include <glib-object.h>
#include <uca/uca-camera.h>

G_BEGIN_DECLS

#define UCA_TYPE_PCOWIN_CAMERA             (uca_pcowin_camera_get_type())
#define UCA_PCOWIN_CAMERA(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), UCA_TYPE_PCOWIN_CAMERA, UcaPcowinCamera))
#define UCA_IS_PCOWIN_CAMERA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), UCA_TYPE_PCOWIN_CAMERA))
#define UCA_PCOWIN_CAMERA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), UFO_TYPE_PCOWIN_CAMERA, UfoPcowinCameraClass))
#define UCA_IS_PCOWIN_CAMERA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), UCA_TYPE_PCOWIN_CAMERA))
#define UCA_PCOWIN_CAMERA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), UCA_TYPE_PCOWIN_CAMERA, UcaPcowinCameraClass))

#define UCA_PCOWIN_CAMERA_ERROR uca_pcowin_camera_error_quark()
typedef enum {
    UCA_PCOWIN_CAMERA_ERROR_SDK_INIT,
    UCA_PCOWIN_CAMERA_ERROR_GETTER,
    UCA_PCOWIN_CAMERA_ERROR_SETTER,
    UCA_PCOWIN_CAMERA_ERROR_UNSUPPORTED,
    UCA_PCOWIN_CAMERA_ERROR_SDKERROR,
    UCA_PCOWIN_CAMERA_ERROR_GENERAL,
} UcaPcoCameraError;

typedef struct _UcaPcowinCamera           UcaPcowinCamera;
typedef struct _UcaPcowinCameraClass      UcaPcowinCameraClass;
typedef struct _UcaPcowinCameraPrivate    UcaPcowinCameraPrivate;

typedef enum {
    UCA_PCO_CAMERA_RECORD_MODE_SEQUENCE,
    UCA_PCO_CAMERA_RECORD_MODE_RING_BUFFER,
} UcaPcoCameraRecordMode;

typedef enum {
    UCA_PCO_CAMERA_STORAGE_MODE_RECORDER,
    UCA_PCO_CAMERA_STORAGE_MODE_FIFO_BUFFER,
} UcaPcoCameraStorageMode;

typedef enum {
    UCA_PCO_CAMERA_ACQUIRE_MODE_AUTO,
    UCA_PCO_CAMERA_ACQUIRE_MODE_EXTERNAL
} UcaPcoCameraAcquireMode;

typedef enum {
    UCA_PCO_CAMERA_TIMESTAMP_NONE,
    UCA_PCO_CAMERA_TIMESTAMP_BINARY,
    UCA_PCO_CAMERA_TIMESTAMP_BINARYANDASCII,
    UCA_PCO_CAMERA_TIMESTAMP_ASCII
} UcaPcoCameraTimestamp;

/**
 * UcaPcowinCamera:
 *
 * Creates #UcaPcowinCamera instances by loading corresponding shared objects. The
 * contents of the #UcaPcowinCamera structure are private and should only be
 * accessed via the provided API.
 */
struct _UcaPcowinCamera {
    /*< private >*/
    UcaCamera parent;

    UcaPcowinCameraPrivate *priv;
};

/**
 * UcaPcowinCameraClass:
 *
 * #UcaPcowinCamera class
 */
struct _UcaPcowinCameraClass {
    /*< private >*/
    UcaCameraClass parent;
};

GType uca_pcowin_camera_get_type(void);

G_END_DECLS

#endif
