// Stub for com.google.vr.sdk.samples.permission.PermissionHelper
// UE 5.4 GameActivity.java.template imports this unconditionally, but the
// GoogleVR SDK is deprecated and no longer distributed as an AAR/Maven dependency.
// This minimal stub provides the two methods the template actually calls:
//   - checkPermission(String): boolean
//   - acquirePermissions(String[], Activity): void
// It simply delegates to android.content.pm.PackageManager so behavior is
// equivalent to what the original helper did, without the VR SDK dependency.
package com.google.vr.sdk.samples.permission;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

public final class PermissionHelper
{
    private PermissionHelper() {}

    /** Returns true if the given permission is already granted. */
    public static boolean checkPermission(String permission)
    {
        try
        {
            // GameActivity calls this in a static context before the Activity is
            // available; fallback to checking against the package manager of the
            // current process via the VR SDK's original API surface.
            Context ctx = null;
            try
            {
                ctx = (Context) Class.forName("android.app.ActivityThread")
                    .getMethod("currentApplication")
                    .invoke(null);
            }
            catch (Throwable ignored) {}
            if (ctx == null)
            {
                return true; // best-effort: allow startup if no context yet
            }
            return ctx.checkPermission(permission, android.os.Process.myPid(), android.os.Process.myUid())
                == PackageManager.PERMISSION_GRANTED;
        }
        catch (Throwable t)
        {
            return true;
        }
    }

    /** Overload used by GameActivity (acquire permissions at runtime). */
    public static void acquirePermissions(String[] permissions, Activity activity)
    {
        if (activity == null || permissions == null) return;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            activity.requestPermissions(permissions, 0);
        }
    }

    /** Overload kept for completeness; unused by the template. */
    public static boolean isPermissionGranted(Activity activity, String permission)
    {
        if (activity == null || permission == null) return true;
        return activity.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED;
    }
}
