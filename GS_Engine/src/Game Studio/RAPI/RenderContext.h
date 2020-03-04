#pragma once

#include "Core.h"
#include "Utility/Extent.h"
#include "Containers/FVector.hpp"

namespace RAPI
{
	class Window;
	class RenderDevice;
	class RenderTarget;

	struct ResizeInfo
	{
		RenderDevice* RenderDevice = nullptr;
		Extent2D NewWindowSize;
	};

	struct RenderContextCreateInfo
	{
		Window* Window = nullptr;
		uint8 DesiredFramesInFlight = 0;
	};
	
	class RenderContext
	{
	protected:
		uint8 currentImage = 0;
		uint8 maxFramesInFlight = 0;

		Extent2D extent{ 0, 0 };

	public:
		virtual ~RenderContext()
		{
		};

		virtual void OnResize(const ResizeInfo& _RI) = 0;

		struct AcquireNextImageInfo : RenderInfo
		{
		};
		virtual void AcquireNextImage(const AcquireNextImageInfo& acquireNextImageInfo);

		struct FlushInfo : RenderInfo
		{
		};
		virtual void Flush(const FlushInfo& flushInfo);

		struct PresentInfo : RenderInfo
		{
		};
		virtual void Present(const PresentInfo& presentInfo);

		[[nodiscard]] virtual FVector<RenderTarget*> GetSwapchainImages() const = 0;

		[[nodiscard]] uint8 GetCurrentImage() const { return currentImage; }
		[[nodiscard]] uint8 GetMaxFramesInFlight() const { return maxFramesInFlight; }
	};
}
