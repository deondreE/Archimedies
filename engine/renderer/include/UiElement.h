#pragma once

namespace arch::core::ui
{
    struct Rect
    {
        float x,y,w,h;

        /**
         * Checks if a point (px, py) resides within this rectangle.
         * Assumes x,y is the top-left corner.
         */
        [[nodiscard]] bool Contains(float px, float py) const
        {
            return (px >= x && px <= (x + w) && 
                    py >= y && py <= (y + h));
        }
    };

    class IViewport
    {
    public:
       virtual ~IViewport() = default;
        virtual void SetBounds(Rect bounds) = 0;
        virtual Rect GetBounds() const = 0;
        virtual void SetTitle(const std::string& title) = 0;
        virtual std::string GetTitle() const = 0;

          /**
         * Returns the API-specific texture handle (e.g., id<MTLTexture> for Metal).
         */
        virtual void* GetNativeRenderTarget() = 0;

        virtual void OnUpdate(float dt) = 0;
        virtual bool OnMouseMoved(float x, float y) = 0;
    };

    class IUIManager
    {
    public:
        virtual ~IUIManager() = default;
        virtual void Initialize() = 0;
         
        /**
         * Recalculates the bounds of all viewports based on the screen size
         * and docking logic.
         */
        virtual void Layout(float screenWidth, float screenHeight) = 0;
        virtual void AddViewport(std::shared_ptr<IViewport> viewport) = 0;
        virtual const std::vector<std::shared_ptr<IViewport>>& GetViewports() const = 0;
    };
}