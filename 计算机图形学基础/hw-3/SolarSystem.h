#ifndef SOLARSYSTEM_H_INCLUDED
#define SOLARSYSTEM_H_INCLUDED

#include <map>
#include <memory>
#include <string>

#include "./lib/Sphere.h"
#include "./lib/Text.h"

class CelestialObject
{
protected:
    Sphere planet;
    float diameter;

public:
    CelestialObject(const std::string& texture, float diameter):
        planet(texture), diameter(diameter)
    {}

    // declare empty virtual destructor
    virtual ~CelestialObject() {}

    glm::vec3 top() const
    {
        return planet.top();
    }

    void draw(glm::mat4 projection, glm::mat4 view)
    {
        planet.draw(projection, view);
    }

    virtual void step(float dt)
    {
        auto S = glm::scale(glm::mat4(1.0f), glm::vec3(diameter));
        auto M = S * glm::mat4(1.0f);
        planet.setModel(M);
    }
};

class Planet : public CelestialObject
{
public:
    const float orbitRadius;
    const float orbitSpeed;
    const float rotationSpeed;

    float axisAngle = 0.0f;
    float orbitAngle = 0.0f;

public:
    Planet(const std::string& texture, float diameter, float orbitRadius, float orbitSpeed, float rotationSpeed):
        CelestialObject(texture, diameter),
        orbitRadius(orbitRadius),
        orbitSpeed(orbitSpeed),
        rotationSpeed(rotationSpeed)
    {}

    virtual void step(float dt) override
    {
        axisAngle  += rotationSpeed * dt;
        orbitAngle += orbitSpeed * dt;

        auto S = glm::scale(glm::mat4(1.0f), glm::vec3(diameter));
        auto R_axis = glm::rotate(glm::mat4(1.0f), glm::radians(axisAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        auto T_sun = glm::translate(glm::mat4(1.0f), glm::vec3(orbitRadius * 100.0f, 0.0f, 0.0f));
        auto R_sun = glm::rotate(glm::mat4(1.0f), glm::radians(orbitAngle), glm::vec3(0.0f, 1.0f, 0.0f));

        auto M = R_sun * T_sun * R_axis * S * glm::mat4(1.0f);

        planet.setModel(M);
    }
};

class Moon : public Planet
{
private:
    float moonSpeed;
    float smallOrbitAngle = 0.0f;

public:
    Moon(const std::string& texture, float diameter, float rotationSpeed, const Planet& parent, float moonSpeed):
        Planet(texture, diameter, parent.orbitRadius, parent.orbitSpeed, rotationSpeed),
        moonSpeed(moonSpeed)
    {}

    virtual void step(float dt) override
    {
        axisAngle       += rotationSpeed * dt;
        orbitAngle      += orbitSpeed * dt;
        smallOrbitAngle += moonSpeed * dt;

        auto S = glm::scale(glm::mat4(1.0f), glm::vec3(diameter));
        auto R_axis = glm::rotate(glm::mat4(1.0f), glm::radians(axisAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        auto T_earth = glm::translate(glm::mat4(1.0f), glm::vec3(-0.16f, 0.0f, 0.0f));
        auto R_parent = glm::rotate(glm::mat4(1.0f), glm::radians(smallOrbitAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        auto T_sun = glm::translate(glm::mat4(1.0f), glm::vec3(orbitRadius * 100.0f, 0.0f, 0.0f));
        auto R_sun = glm::rotate(glm::mat4(1.0f), glm::radians(orbitAngle), glm::vec3(0.0f, 1.0f, 0.0f));

        auto M = R_sun * T_sun * R_parent * T_earth * R_axis * S * glm::mat4(1.0f);

        planet.setModel(M);
    }
};

class SolarSystem
{
    const glm::vec3 labelColor = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 labelScale = glm::vec3(0.4f);

private:
    float speed = 1.0f;
    std::map<std::string, std::unique_ptr<CelestialObject>> planets;
    std::map<std::string, Text> labels;

public:
    SolarSystem()
    {
        planets.insert({"sun", std::make_unique<CelestialObject>("./textures/sun.jpg", 1400000.0f / 1400000.0f)});
        planets.insert({"mercury", std::make_unique<Planet>("./textures/mercury.jpg" , 4878.0f    / 139822.0f  , 70000000.0f   / 4500000000.0f , 47.8f , 10.0f)});
        planets.insert({"venus"  , std::make_unique<Planet>("./textures/venus.jpg"   , 12104.0f   / 139822.0f  , 109000000.0f  / 4500000000.0f , 35.0f , 10.0f)});
        planets.insert({"earth"  , std::make_unique<Planet>("./textures/earth.jpg"   , 12756.0f   / 139822.0f  , 152000000.0f  / 4500000000.0f , 29.8f , 10.0f)});
        planets.insert({"mars"   , std::make_unique<Planet>("./textures/mars.jpg"    , 6780.0f    / 139822.0f  , 249000000.0f  / 4500000000.0f , 24.2f , 10.0f)});
        planets.insert({"jupiter", std::make_unique<Planet>("./textures/jupiter.jpg" , 139822.0f  / 139822.0f  , 817000000.0f  / 4500000000.0f , 13.1f , 10.0f)});
        planets.insert({"saturn" , std::make_unique<Planet>("./textures/saturn.jpg"  , 116464.0f  / 139822.0f  , 1500000000.0f / 4500000000.0f , 9.68f , 10.0f)});
        planets.insert({"uranus" , std::make_unique<Planet>("./textures/uranus.jpg"  , 50724.0f   / 139822.0f  , 3000000000.0f / 4500000000.0f , 6.6f  , 10.0f)});
        planets.insert({"neptune", std::make_unique<Planet>("./textures/neptune.jpg" , 49248.0f   / 139822.0f  , 4500000000.0f / 4500000000.0f , 5.4f  , 10.0f)});
        planets.insert({"moon", std::make_unique<Moon>("./textures/moon.jpg", 3476.0f / 139822.0f, 0.0f, dynamic_cast<Planet&>(*planets.at("earth")), 111.0f)});

        labels.insert({"sun"    , Text("Sun"    , labelColor, labelScale)});
        labels.insert({"mercury", Text("Mercury", labelColor, labelScale)});
        labels.insert({"venus"  , Text("Venus"  , labelColor, labelScale)});
        labels.insert({"earth"  , Text("Earth"  , labelColor, labelScale)});
        labels.insert({"mars"   , Text("Mars"   , labelColor, labelScale)});
        labels.insert({"jupiter", Text("Jupiter", labelColor, labelScale)});
        labels.insert({"saturn" , Text("Saturn" , labelColor, labelScale)});
        labels.insert({"uranus" , Text("Uranus" , labelColor, labelScale)});
        labels.insert({"neptune", Text("Neptune", labelColor, labelScale)});
    }

    void increaseSpeed(float dt) { speed += 2.0f * dt; }
    void decreaseSpeed(float dt) { speed -= 2.0f * dt; }

    void draw(glm::mat4 projection, glm::mat4 view) const
    {
        for (auto& p : planets)
            p.second->draw(projection, view);

        for (auto& l : labels)
            l.second.draw(projection, view);
    }

    void step(float dt)
    {
        for (auto& p : planets)
            p.second->step(dt * speed);

        repositionLabels();
    }

    void repositionLabels()
    {
        // make sure text is always facing camera
        auto source = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
        auto target = glm::normalize(Camera::getRight());
        float angle_rad = acos(glm::dot(source, target));
        if ((target - source).z > 0) angle_rad *= -1.0f;

        glm::mat4 R;
        R = glm::rotate(R, angle_rad, glm::vec3(0.0f, 1.0f, 0.0f));
        R = glm::rotate(R, glm::radians(Camera::getPitch()), glm::vec3(1.0f, 0.0f, 0.0f));

        for (auto& entry : labels)
        {
            std::string name = entry.first;
            Text& label = entry.second;

            label.resetTranslate();
            label.translate(planets.at(name)->top() + glm::vec3(-0.2f, 0.1f, 0.0f));
            label.setRotate(R);
        }
    }
};

#endif // SOLARSYSTEM_H_INCLUDED