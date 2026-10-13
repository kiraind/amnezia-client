package org.amnezia.vpn

import android.system.Os
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.Prefs
import org.qtproject.qt.android.bindings.QtApplication

private const val TAG = "AmneziaApplication"

class AmneziaApplication : QtApplication() {

    override fun onCreate() {
        if (BuildConfig.DEBUG) {
            Os.setenv("QT_ANDROID_DEBUGGER_MAIN_THREAD_SLEEP_MS", "0", true)
        }
        super.onCreate()
        Prefs.init(this)
        Log.init(this)
        VpnStateStore.init(this)
        Log.d(TAG, "Create Amnezia application")
        ServiceNotification.createNotificationChannel(this)
    }
}
