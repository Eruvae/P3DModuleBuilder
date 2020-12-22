#include <pandabase.h>
#include <geom.h>
#include <geomVertexData.h>
#include <geomVertexWriter.h>
#include <geomTriangles.h>
#include <geomPrimitive.h>
#include <iostream>

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