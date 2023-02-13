#include "pch.h"
#include "PhotoEditor.Photo.h"
#include "PhotoEditor.Photo.g.cpp"


namespace winrt::PhotoEditor::implementation
{
    Photo::Photo(winrt::Windows::Storage::StorageFile const& imageFile)
    {
        throw hresult_not_implemented();
    }
    hstring Photo::ImageName()
    {
        throw hresult_not_implemented();
    }
    float Photo::SepiaIntensity()
    {
        throw hresult_not_implemented();
    }
    void Photo::SepiaIntensity(float value)
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::IAsyncAction Photo::StartRecognitionAsync()
    {
        throw hresult_not_implemented();
    }
    winrt::event_token Photo::ImageRecognized(winrt::PhotoEditor::RecognitionHandler const& handler)
    {
        throw hresult_not_implemented();
    }
    void Photo::ImageRecognized(winrt::event_token const& token) noexcept
    {
        throw hresult_not_implemented();
    }
    winrt::event_token Photo::PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        throw hresult_not_implemented();
    }
    void Photo::PropertyChanged(winrt::event_token const& token) noexcept
    {
        throw hresult_not_implemented();
    }
}
