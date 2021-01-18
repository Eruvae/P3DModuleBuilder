#include <pandabase.h>
#include <geom.h>
#include <geomVertexData.h>
#include <geomVertexWriter.h>
#include <geomTriangles.h>
#include <geomPrimitive.h>
#include <iostream>
#include <unordered_map>

template <typename T>
T rol_(T value, int count) {
    return (value << count) | (value >> (sizeof(T)*CHAR_BIT - count));
}

template <typename T>
T ror_(T value, int count) {
    return (value >> count) | (value << (sizeof(T)*CHAR_BIT - count));
}

namespace std {

  template <>
  struct hash<LVecBase3i>
  {
    std::size_t operator()(const LVecBase3i& k) const
    {
      return -rol_((std::size_t)k[0], 16) ^ -rol_((std::size_t)k[1], 32) ^ -rol_((std::size_t)k[2], 48);
    }
  };

}

class VoxelGrid
{
    LVecBase3i shape;
    size_t y_stride;
    size_t z_stride;
    double scale;
    PTA(float) colors;
    std::unordered_map<LVecBase3i, size_t> vertex_indices;
    size_t ci;
    
    PT(GeomVertexData) vertices;
    GeomVertexWriter vertex_writer;
    GeomVertexWriter color_writer;
    PT(Geom) geom;
    PT(GeomTriangles) tri_prim;

    inline LVecBase4 getColor(int value)
    {
        size_t col_ind = (value - 1) * 4;
        return LVecBase4(colors[col_ind], colors[col_ind + 1], colors[col_ind + 2], colors[col_ind + 3]);   
    }

    void addCube(const LVecBase3i &coord, int value)
    {
        LVecBase3 min(coord[0] * scale, coord[1] * scale, coord[2] * scale);
        LVecBase3 max((coord[0] + 1) * scale, (coord[1] + 1) * scale, (coord[2] + 1) * scale);

        vertex_writer.add_data3(min[0], min[1], min[2]);
        vertex_writer.add_data3(max[0], min[1], min[2]);
        vertex_writer.add_data3(max[0], max[1], min[2]);
        vertex_writer.add_data3(min[0], max[1], min[2]);
        vertex_writer.add_data3(min[0], max[1], max[2]);
        vertex_writer.add_data3(max[0], max[1], max[2]);
        vertex_writer.add_data3(max[0], min[1], max[2]);
        vertex_writer.add_data3(min[0], min[1], max[2]);
        

        for (int i = 0; i < 8; i++)
            color_writer.add_data4(getColor(value));

        tri_prim->add_vertices(ci + 0, ci + 2, ci + 1);
        tri_prim->add_vertices(ci + 0, ci + 3, ci + 2);
        tri_prim->add_vertices(ci + 2, ci + 3, ci + 4);
        tri_prim->add_vertices(ci + 2, ci + 4, ci + 5);
        tri_prim->add_vertices(ci + 1, ci + 2, ci + 5);
        tri_prim->add_vertices(ci + 1, ci + 5, ci + 6);
        tri_prim->add_vertices(ci + 0, ci + 7, ci + 4);
        tri_prim->add_vertices(ci + 0, ci + 4, ci + 3);
        tri_prim->add_vertices(ci + 5, ci + 4, ci + 7);
        tri_prim->add_vertices(ci + 5, ci + 7, ci + 6);
        tri_prim->add_vertices(ci + 0, ci + 6, ci + 7);
        tri_prim->add_vertices(ci + 0, ci + 1, ci + 6);

        vertex_indices[coord] = ci;
        ci += 8;
    }

PUBLISHED:
    VoxelGrid(const LVecBase3i &shape, PTA(float) colors, double scale) : shape(shape), y_stride(shape[0]), z_stride(shape[0]*shape[1]),
            scale(scale), colors(colors), ci(0),
            vertices(new GeomVertexData("vertices", GeomVertexFormat::get_v3c4(), Geom::UH_dynamic)),
            vertex_writer(vertices, "vertex"), color_writer(vertices, "color"), geom(new Geom(vertices)),
            tri_prim(new GeomTriangles(Geom::UH_dynamic))

    {
        geom->add_primitive(tri_prim);
    }

    VoxelGrid(PTA(int) data, const LVecBase3i &shape, PTA(float) colors, double scale) : shape(shape), y_stride(shape[0]),
            z_stride(shape[0]*shape[1]), scale(scale), colors(colors), ci(0),
            vertices(new GeomVertexData("vertices", GeomVertexFormat::get_v3c4(), Geom::UH_dynamic)),
            vertex_writer(vertices, "vertex"), color_writer(vertices, "color"), geom(new Geom(vertices)),
            tri_prim(new GeomTriangles(Geom::UH_dynamic))
    {
        geom->add_primitive(tri_prim);
        reset(data);
    }

    void reset(PTA(int) data)
    {
        vertex_indices.clear();
        tri_prim->clear_vertices();
        vertices->clear_rows();
        vertex_writer.set_row(0);
        color_writer.set_row(0);
        ci = 0;

        if (data.size() != shape[0] * shape[1] * shape[2])
            data.resize(shape[0] * shape[1] * shape[2]);

        /*size_t voxels = 0;
        for (int i : data)
        {
            if (i != 0)
                voxels++;
        }
        vertices->set_num_rows(voxels);*/
        //vertices->set_num_rows(data.size());

        std::cout << "Generating voxel grid" << std::endl;
        for (int z = 0; z < shape[2]; z++)
        {
            if (z % 10 == 0)
                std::cout << "Progress: " << ((float)z / (float)shape[2]) << std::endl;
            
            for (int y = 0; y < shape[1]; y++)
            {
                for (int x = 0; x < shape[0]; x++)
                {
                    int value = data[z * z_stride + y * y_stride + x];
                    if (value != 0)
                    {
                        LVecBase3i coord(x, y, z);
                        addCube(coord, value);
                    }
                }
            }
        }
        std::cout << "Vertices: " << vertices->get_num_rows() << ", Faces: " << tri_prim->get_num_faces() << std::endl;
    }

    void updateValue(const LVecBase3i &coord, int value)
    {
        auto it = vertex_indices.find(coord);
        if (it != vertex_indices.end())
        {
            color_writer.set_row(it->second);
            for (int i = 0; i < 8; i++)
                color_writer.set_data4(getColor(value));
        }
        else
        {
            color_writer.set_row(ci);
            addCube(coord, value);
        }   
    }

    void updateValues(PTA(int) indices, PTA(int) values)
    {
        for (size_t i = 0; i < values.size(); i++)
        {
            LVecBase3i coord(indices[3*i], indices[3*i + 1], indices[3*i + 2]);
            auto it = vertex_indices.find(coord);
            if (it != vertex_indices.end())
            {
                color_writer.set_row(it->second);
                for (int j = 0; j < 8; j++)
                    color_writer.set_data4(getColor(values[i]));
            }
            else
            {
                color_writer.set_row(ci);
                addCube(coord, values[i]);
            }
        }
    }

    /*void updateValues(PTA(int) indices, int value)
    {
        for (size_t i = 0; i < indices.size() - 2; i += 3)
        {
            LVecBase3i coord(indices[i], indices[i + 1], indices[i + 2]);
            auto it = vertex_indices.find(coord);
            if (it != vertex_indices.end())
            {
                color_writer.set_row(it->second);
                for (int i = 0; i < 8; i++)
                    color_writer.set_data4(getColor(value));
            }
            else
            {
                color_writer.set_row(ci);
                addCube(coord, value);
            }
        }
    }*/

    PT(Geom) getGeom()
    {
        return geom;
    }
};

BEGIN_PUBLISH

PT(Geom) create_voxel_grid(const BitArray &arr, const LVecBase3i &arr_shape, float scale, const LVecBase4 &min_col, const LVecBase4 &max_col)
{
    const int vertex_shape[3] = { arr_shape[0] + 1, arr_shape[1] + 1, arr_shape[2] + 1 };
    size_t y_stride = vertex_shape[0];
    size_t z_stride = vertex_shape[0] * vertex_shape[1];
    size_t num_vertices = vertex_shape[0] * vertex_shape[1] * vertex_shape[2];
    PT(GeomVertexData) vertices(new GeomVertexData("vertices", GeomVertexFormat::get_v3c4(), Geom::UH_static));
    vertices->set_num_rows(num_vertices);
    GeomVertexWriter vertex(vertices, "vertex");
    GeomVertexWriter color(vertices, "color");

    std::cout << "Generating vertices..." << std::endl;

    for (size_t z = 0; z < vertex_shape[2]; z++)
    {
        for (size_t y = 0; y < vertex_shape[1]; y++)
        {
            for (size_t x = 0; x < vertex_shape[1]; x++)
            {
                double prog = (double)z / (double)vertex_shape[2]; //(x / vertex_shape[0] + y / vertex_shape[1] + z / vertex_shape[2]) / 3
                LVecBase4 col = min_col * (1 - prog) + max_col * prog;
                vertex.add_data3(x * scale, y * scale, z * scale);
                color.add_data4(col[0], col[1], col[2], col[3]);
            }
        }
    }

    PT(Geom) geom(new Geom(vertices));
    PT(GeomTriangles) tri_prim(new GeomTriangles(Geom::UH_static));

    std::cout << "Generating faces..." << std::endl;

    for (int z = 0; z < arr_shape[2]; z++)
    {
        for (int y = 0; y < arr_shape[1]; y++)
        {
            for (int x = 0; x < arr_shape[0]; x++)
            {
                if (arr.get_bit(x + y * arr_shape[0] + z * arr_shape[0] * arr_shape[1]))
                {
                    size_t coord[8] = {
                        x + y * y_stride + z * z_stride,
                        (x + 1) + y * y_stride + z * z_stride,
                        (x + 1) + (y + 1) * y_stride + z * z_stride,
                        x + (y + 1) * y_stride + z * z_stride,
                        x + (y + 1) * y_stride + (z + 1) * z_stride,
                        (x + 1) + (y + 1) * y_stride + (z + 1) * z_stride,
                        (x + 1) + y * y_stride + (z + 1) * z_stride,
                        x + y * y_stride + (z + 1) * z_stride,
                    };

                    size_t triangles[12][3] = {
                        {coord[0], coord[2], coord[1]},
                        {coord[0], coord[3], coord[2]},
                        {coord[2], coord[3], coord[4]},
                        {coord[2], coord[4], coord[5]},
                        {coord[1], coord[2], coord[5]},
                        {coord[1], coord[5], coord[6]},
                        {coord[0], coord[7], coord[4]},
                        {coord[0], coord[4], coord[3]},
                        {coord[5], coord[4], coord[7]},
                        {coord[5], coord[7], coord[6]},
                        {coord[0], coord[6], coord[7]},
                        {coord[0], coord[1], coord[6]}
                    };

                    for (const auto& tri : triangles)
                        tri_prim->add_vertices(tri[0], tri[1], tri[2]);
                }
            }
        }
    }

    geom->add_primitive(tri_prim);

    return geom;
}

END_PUBLISH