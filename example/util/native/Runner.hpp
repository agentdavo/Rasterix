#include <stdio.h>
#include <spdlog/spdlog.h>
#include "FT60XBusConnector.hpp"
#include "IceGL.hpp"
#include "renderer/Renderer.hpp"
#include "RenderConfigs.hpp"
#include "gl.h"
#include "glu.h"


template <typename Scene>
class Runner
{
public:
    Runner() 
    {
        spdlog::set_level(spdlog::level::trace);
        rr::IceGL::createInstance(m_renderer);
        m_renderer.setRenderResolution(RESOLUTION_W, RESOLUTION_H);
    }

    void execute()
    {
        m_scene.init(RESOLUTION_W, RESOLUTION_H);
        while (1)
        {
            m_scene.draw();
            rr::IceGL::getInstance().render();
        }
    }
private:
    static constexpr uint32_t RESOLUTION_H = 600;
    static constexpr uint32_t RESOLUTION_W = 1024;
    rr::FT60XBusConnector m_busConnector;
#if VARIANT_RRXIF == 1
    rr::Renderer<rr::RenderConfigRRXIFNexys> m_renderer { m_busConnector };
#endif // VARIANT_RRXIF
#if VARIANT_RRXEF == 1
    rr::Renderer<rr::RenderConfigRRXEFNexys> m_renderer { m_busConnector };
#endif // VARIANT_RRXEF
    Scene m_scene {};
};
