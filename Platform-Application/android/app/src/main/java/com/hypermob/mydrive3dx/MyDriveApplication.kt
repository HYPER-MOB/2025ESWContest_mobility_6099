package com.hypermob.mydrive3dx

import android.app.Application
import dagger.hilt.android.HiltAndroidApp

/**
 * MyDrive3DX Application 클래스
 *
 * Hilt DI를 위한 진입점
 */
@HiltAndroidApp
class MyDriveApplication : Application() {

    override fun onCreate() {
        super.onCreate()

        // TODO: 초기화 로직 추가
        // - Firebase 초기화
        // - 루팅 감지 (RootBeer)
        // - 앱 보안 설정
    }
}
