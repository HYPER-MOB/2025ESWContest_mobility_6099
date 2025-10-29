package com.hypermob.mydrive3dx.presentation.face

import android.graphics.Bitmap
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hypermob.mydrive3dx.domain.model.FaceRegisterResult
import com.hypermob.mydrive3dx.domain.model.Result
import com.hypermob.mydrive3dx.domain.usecase.RegisterFaceUseCase
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Face Register ViewModel
 * 얼굴 등록 화면의 ViewModel
 *
 * android_mvp의 FaceRegisterActivity 로직을 MVVM 패턴으로 변환
 */
@HiltViewModel
class FaceRegisterViewModel @Inject constructor(
    private val registerFaceUseCase: RegisterFaceUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow<FaceRegisterUiState>(FaceRegisterUiState.Initial)
    val uiState: StateFlow<FaceRegisterUiState> = _uiState.asStateFlow()

    /**
     * 카메라로 캡처한 얼굴 이미지를 서버에 등록
     */
    fun registerFace(bitmap: Bitmap) {
        viewModelScope.launch {
            registerFaceUseCase(bitmap).collect { result ->
                _uiState.value = when (result) {
                    is Result.Loading -> FaceRegisterUiState.Loading
                    is Result.Success -> FaceRegisterUiState.Success(result.data)
                    is Result.Error -> FaceRegisterUiState.Error(
                        result.exception.message ?: "Face registration failed"
                    )
                }
            }
        }
    }

    /**
     * 에러 상태 초기화
     */
    fun clearError() {
        _uiState.value = FaceRegisterUiState.Initial
    }
}

/**
 * Face Register UI State
 */
sealed class FaceRegisterUiState {
    object Initial : FaceRegisterUiState()
    object Loading : FaceRegisterUiState()
    data class Success(val result: FaceRegisterResult) : FaceRegisterUiState()
    data class Error(val message: String) : FaceRegisterUiState()
}
