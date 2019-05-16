#ifndef OBJMODEL_H_INCLUDED
#define OBJMODEL_H_INCLUDED

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>

class OBJModel
{
private:
    std::vector<glm::vec3> vs;  // vertices
    std::vector<glm::ivec3> fs; // faces
    std::vector<glm::vec3> ns;  // normals

public:
    OBJModel(const std::string& path)
    {
        std::ifstream infile(path);

        std::string line;
        while(std::getline(infile, line))
        {
            char type;
            std::istringstream iss(line);
            iss >> type; // assume no error

            if (type == '#') continue;

            if (type == 'v')
            {
                float x, y, z;
                iss >> x >> y >> z;
                vs.push_back(glm::vec3(x, y, z));
            }

            else if (type == 'f')
            {
                unsigned int v1, v2, v3;
                iss >> v1 >> v2 >> v3;
                fs.push_back({v1 - 1, v2 - 1, v3 - 1});
            }
        }
    }

    void calculateNormals()
    {
        ns.resize(vs.size());

        // store incident faces for each vertex
        std::map<int, std::vector<int>> incident;

        for (int f = 0; f < fs.size(); ++f)
        {
            incident[fs[f].x].push_back(f);
            incident[fs[f].y].push_back(f);
            incident[fs[f].z].push_back(f);
        }

        // calculate normals for each vertex
        for (int v = 0; v < vs.size(); ++v)
        {
            for (auto f : incident[v])
            {
                auto v1 = vs[fs[f].x];
                auto v2 = vs[fs[f].y];
                auto v3 = vs[fs[f].z];

                // add normal corresponding to this specific face
                ns[v] += glm::normalize(glm::cross(v1 - v3, v2 - v3));
            }

            ns[v] = glm::normalize(ns[v]);
        }
    }

    const std::vector<glm::vec3>& vertices() const { return vs; }
    const std::vector<glm::ivec3>& faces() const { return fs; }

    const std::vector<glm::vec3>& normals() const
    {
        assert(ns.size() == vs.size());
        return ns;
    }
};

#endif // OBJMODEL_H_INCLUDED