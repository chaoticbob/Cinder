package org.libcinder.hardware;

import org.libcinder.Cinder;

import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.util.Log;
import android.graphics.SurfaceTexture;

import java.util.concurrent.locks.ReentrantLock;

/** \class CameraV1
 *
 */
public class CameraV1 extends org.libcinder.hardware.Camera implements android.hardware.Camera.PreviewCallback {

    private static final String TAG = "CameraV1";

    private SurfaceTexture mDummyTexture = null;

    private android.hardware.Camera mCamera = null;

    private ReentrantLock mPixelsMutex  = null;

    public CameraV1() {
        // @TODO
    }

    public static void checkCameraPresence(boolean[] back, boolean[] front) {
        back[0] = false;
        front[0] = false;
        int numberOfCameras = Camera.getNumberOfCameras();
        for( int i = 0; i < numberOfCameras; ++i ) {
            android.hardware.Camera.CameraInfo info = new android.hardware.Camera.CameraInfo();
            Camera.getCameraInfo(i, info);
            if(CameraInfo.CAMERA_FACING_BACK == info.facing) {
                back[0] = true;
            }
            else if(CameraInfo.CAMERA_FACING_FRONT == info.facing) {
                front[0] = true;
            }
        }
    }

    @Override
    public final void initialize() {
        int numberOfCameras = Camera.getNumberOfCameras();
        for( int i = 0; i < numberOfCameras; ++i ) {
            android.hardware.Camera.CameraInfo info = new android.hardware.Camera.CameraInfo();
            Camera.getCameraInfo(i, info);
            if(CameraInfo.CAMERA_FACING_BACK == info.facing) {
                mBackDeviceId = Integer.toString(i);
            }
            else if(CameraInfo.CAMERA_FACING_FRONT == info.facing) {
                mFrontDeviceId = Integer.toString(i);
            }
        }

/*
        mActiveDeviceId = (-1 != mBackDeviceId) ? mBackDeviceId : ((-1 != mFrontDeviceId) ? mFrontDeviceId : -1);

        Log.i(Cinder.TAG, "Back Camera: " + mBackDeviceId);
        Log.i(Cinder.TAG, "Front Camera: " + mFrontDeviceId);
*/
        if(null == mDummyTexture) {
            mDummyTexture = new SurfaceTexture(0);
        }

        mPixelsMutex = new ReentrantLock();
    }

    /** startDevice
     *
     */
    private void startDevice() {
        // Bail if we don't have a valid camera id or mCamera isn't null
        if ((null == mActiveDeviceId) || (null != mCamera)) {
            return;
        }

        try {
            mCamera = android.hardware.Camera.open(Integer.parseInt(mActiveDeviceId));

            Camera.Parameters params = mCamera.getParameters();
            mWidth = params.getPreviewSize().width;
            mHeight = params.getPreviewSize().height;

            mCamera.setPreviewTexture(mDummyTexture);
            mCamera.setPreviewCallback(this);
            mCamera.startPreview();
        }
        catch(Exception e ) {
            Log.e(Cinder.TAG, "CinderCamera.startDevice failed: " + e);
        }
    }

    /** stopDevice
     *
     */
    private void stopDevice() {
        try {
            if (null != mCamera) {
                mCamera.setPreviewTexture(null);
                mCamera.setPreviewCallback(null);
                mCamera.stopPreview();
                mCamera.release();
                mCamera = null;
            }
        }
        catch(Exception e ) {
            Log.e(Cinder.TAG, "CinderCamera.stopDevice failed: " + e);
        }
    }

    /** startBackDevice
     *
     */
    private void startBackDevice() {
        if((null != mActiveDeviceId) && (mActiveDeviceId.equals(mBackDeviceId))) {
            return;
        }

        stopDevice();

        mActiveDeviceId = mBackDeviceId;
        startDevice();
    }

    /** startFrontDevice
     *
     */
    private void startFrontDevice() {
        if((null != mActiveDeviceId) && (mActiveDeviceId.equals(mFrontDeviceId))) {
            return;
        }

        stopDevice();

        mActiveDeviceId = mFrontDeviceId;
        startDevice();
    }

    /** onPreviewFrame
     *
     */
    @Override
    public void onPreviewFrame(byte[] data, android.hardware.Camera camera) {
        lockPixels();
        try {
            mPixels = data;
        }
        finally {
            unlockPixels();
        }
    }

    // =============================================================================================
    // Camera functions
    // =============================================================================================

    /** setDummyTexture
     *
     */
    @Override
    public void setDummyTexture(SurfaceTexture dummyTexture) {
        mDummyTexture = dummyTexture;
        if(null != mCamera) {
            try {
                mCamera.setPreviewTexture(mDummyTexture);
            }
            catch(Exception e) {
                Log.w(TAG, "(setDummyTexture) Camera.setPreviewTexture failed");
            }
        }
    }

    /** startCapture
     *
     */
    @Override
    public void startCapture() {
        if(isBackCameraAvailable()) {
            startBackDevice();
        }
        else if(isFrontCameraAvailable()) {
            startFrontDevice();
        }
    }

    /** stopCapture
     *
     */
    @Override
    public void stopCapture() {
        stopDevice();
    }

    /** switchToBackCamera
     *
     */
    @Override
    public void switchToBackCamera() {
        if((null != mActiveDeviceId) && (mActiveDeviceId.equals(mBackDeviceId))) {
            return;
        }

        startBackDevice();
    }

    /** switchToFrontCamera
     *
     */
    @Override
    public void switchToFrontCamera() {
        if((null != mActiveDeviceId) && (mActiveDeviceId.equals(mFrontDeviceId))) {
            return;
        }

        startFrontDevice();
    }

    /** lockPixels
     *
     */
    @Override
    public byte[] lockPixels() {
        mPixelsMutex.lock();
        return mPixels;
    }

    /** unlockPixels
     *
     */
    @Override
    public void unlockPixels() {
        mPixelsMutex.unlock();
    }

/*
    // =============================================================================================
    // Static Methods for C++
    // =============================================================================================

    private static CameraV1 sCamera = null;

    public static boolean initialize() {
        if(null == sCamera) {
            sCamera = new CameraV1();
        }
        return (-1 != sCamera.mFrontDeviceId || -1 != sCamera.mBackDeviceId);
    }

    public static boolean hasFrontCamera() {
        return (null != sCamera) && (-1 != sCamera.mFrontDeviceId);
    }

    public static boolean hasBackCamera() {
        return (null != sCamera) && (-1 != sCamera.mBackDeviceId);
    }

    public static void startCapture() {
        if(null == sCamera) {
            return;
        }

        sCamera.startDevice();
    }

    public static void stopCapture() {
        if(null == sCamera) {
            return;
        }

        sCamera.stopDevice();
    }

    public static byte[] lockPixels() {
        if(null == sCamera) {
            return null;
        }

        sCamera.privateLockPixels();
        return sCamera.mPixels;
    }

    public static void unlockPixels() {
        if(null == sCamera) {
            return;
        }

        sCamera.privateUnlockPixels();
    }

    public static int getWidth() {
        return (null != sCamera) ? sCamera.mWidth : 0;
    }

    public static int getHeight() {
        return (null != sCamera) ? sCamera.mHeight : 0;
    }

    public static void takePicture() {
        if(null == sCamera) {
            return;
        }
        sCamera.takePicture();
    }
*/
}