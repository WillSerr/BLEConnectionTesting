#pragma once
#include "PhotoEditor.Photo.g.h"

namespace winrt::PhotoEditor::implementation
{
    struct Photo : PhotoT<Photo>
    {
        Photo() = default;

        Photo(winrt::Windows::Storage::StorageFile const& imageFile);
        hstring ImageName();
        float SepiaIntensity();
        void SepiaIntensity(float value);
        winrt::Windows::Foundation::IAsyncAction StartRecognitionAsync();
        winrt::event_token ImageRecognized(winrt::PhotoEditor::RecognitionHandler const& handler);
        void ImageRecognized(winrt::event_token const& token) noexcept;
        winrt::event_token PropertyChanged(winrt::Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;
    };
}
namespace winrt::PhotoEditor::factory_implementation
{
    struct Photo : PhotoT<Photo, implementation::Photo>
    {
    };
}
