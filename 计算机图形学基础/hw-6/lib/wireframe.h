#ifndef WIREFRAME_H_INCLUDED
#define WIREFRAME_H_INCLUDED

class Wireframe
{
protected:
    enum class RenderMode {
        WIREFRAME,
        VERTEX,
        FACE
    };

    RenderMode renderMode = RenderMode::FACE;

    void setPolygonMode(RenderMode mode) const
    {
        switch (mode)
        {
            case RenderMode::WIREFRAME:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(5.0f);
                break;

            case RenderMode::VERTEX:
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                glPointSize(5.0f);
                break;

            case RenderMode::FACE:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
        }
    }

public:
    void stepMode()
    {
        switch (renderMode)
        {
            case RenderMode::FACE:      renderMode = RenderMode::WIREFRAME; break;
            case RenderMode::WIREFRAME: renderMode = RenderMode::VERTEX   ; break;
            case RenderMode::VERTEX:    renderMode = RenderMode::FACE     ; break;
        }
    }

    virtual ~Wireframe() {}
};

#endif // WIREFRAME_H_INCLUDED