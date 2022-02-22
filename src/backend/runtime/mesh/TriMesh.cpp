#include "TriMesh.h"
#include "Tangent.h"

namespace IG {

void TriMesh::fixNormals(bool* hasBadNormals)
{
    // Re-normalize all the values in the OBJ file to handle invalid meshes
    bool bad = false;
    for (auto& n : normals) {
        float len2 = n.squaredNorm();
        if (len2 <= FltEps || std::isnan(len2)) {
            bad = true;
            n   = Vector3f::UnitY();
        } else
            n /= std::sqrt(len2);
    }

    if (hasBadNormals)
        *hasBadNormals = bad;
}

void TriMesh::flipNormals()
{
    for (auto& n : face_normals)
        n = -n;
    for (auto& n : normals)
        n = -n;
}

void TriMesh::scale(float scale)
{
    for (auto& v : vertices)
        v *= scale;
}

void TriMesh::mergeFrom(const TriMesh& src)
{
    size_t idx_offset = indices.size();
    size_t vtx_offset = vertices.size();

    if (idx_offset == 0) {
        *this = src;
        return;
    }

    vertices.insert(vertices.end(), src.vertices.begin(), src.vertices.end());
    normals.insert(normals.end(), src.normals.begin(), src.normals.end());
    texcoords.insert(texcoords.end(), src.texcoords.begin(), src.texcoords.end());
    face_normals.insert(face_normals.end(), src.face_normals.begin(), src.face_normals.end());
    face_inv_area.insert(face_inv_area.end(), src.face_inv_area.begin(), src.face_inv_area.end());

    indices.resize(idx_offset + src.indices.size());
    for (size_t i = 0; i < src.indices.size(); i += 4) {
        indices[idx_offset + i + 0] = src.indices[i + 0] + vtx_offset;
        indices[idx_offset + i + 1] = src.indices[i + 1] + vtx_offset;
        indices[idx_offset + i + 2] = src.indices[i + 2] + vtx_offset;
        indices[idx_offset + i + 3] = src.indices[i + 3]; // ID
    }
}

void TriMesh::replaceID(uint32 m_idx)
{
    for (size_t i = 0; i < indices.size(); i += 4)
        indices[i + 3] = m_idx; // ID
}

void TriMesh::computeFaceAreaOnly(bool* hasBadAreas)
{
    bool bad = false;
    face_inv_area.resize(faceCount());

    const size_t inds = indices.size();
    for (size_t i = 0; i < inds; i += 4) {
        const auto& v0 = vertices[indices[i + 0]];
        const auto& v1 = vertices[indices[i + 1]];
        const auto& v2 = vertices[indices[i + 2]];
        Vector3f N     = (v1 - v0).cross(v2 - v0);
        float lN       = N.norm();
        if (lN < 1e-5f) {
            lN  = 1.0f;
            N   = Vector3f::UnitZ();
            bad = true;
        }
        face_inv_area[i / 4] = 1 / (0.5f * lN);
    }
    if (hasBadAreas)
        *hasBadAreas = bad;
}

void TriMesh::computeFaceNormals(bool* hasBadAreas)
{
    bool bad = false;
    face_normals.resize(faceCount());
    face_inv_area.resize(faceCount());

    const size_t inds = indices.size();
    for (size_t i = 0; i < inds; i += 4) {
        const auto& v0 = vertices[indices[i + 0]];
        const auto& v1 = vertices[indices[i + 1]];
        const auto& v2 = vertices[indices[i + 2]];
        Vector3f N     = (v1 - v0).cross(v2 - v0);
        float lN       = N.norm();
        if (lN < 1e-5f) {
            lN  = 1.0f;
            N   = Vector3f::UnitZ();
            bad = true;
        }
        face_normals[i / 4]  = N / lN;
        face_inv_area[i / 4] = 1 / (0.5f * lN);
    }
    if (hasBadAreas)
        *hasBadAreas = bad;
}

void TriMesh::computeVertexNormals()
{
    normals.resize(faceCount() * 3);
    std::fill(normals.begin(), normals.end(), StVector3f::Zero());

    const size_t inds = indices.size();
    for (size_t i = 0; i < inds; i += 4) {
        auto& n0      = normals[indices[i + 0]];
        auto& n1      = normals[indices[i + 1]];
        auto& n2      = normals[indices[i + 2]];
        const auto& n = face_normals[i / 4];
        n0 += n;
        n1 += n;
        n2 += n;
    }

    for (auto& n : normals) {
        n.normalize();
    }
}

void TriMesh::makeTexCoordsZero()
{
    texcoords.resize(vertices.size());
    std::fill(texcoords.begin(), texcoords.end(), Vector2f::Zero());
}

void TriMesh::setupFaceNormalsAsVertexNormals()
{
    // Copy triangle vertices such that each face is unique
    std::vector<StVector3f> new_vertices;
    new_vertices.resize(faceCount() * 3);
    for (size_t f = 0; f < faceCount(); ++f) {
        new_vertices[3 * f + 0] = vertices[indices[4 * f + 0]];
        new_vertices[3 * f + 1] = vertices[indices[4 * f + 1]];
        new_vertices[3 * f + 2] = vertices[indices[4 * f + 2]];
    }
    vertices = std::move(new_vertices);

    // Use face normals for each vertex
    std::vector<StVector3f> new_normals;
    new_normals.resize(faceCount() * 3);
    for (size_t f = 0; f < faceCount(); ++f) {
        const StVector3f facenormal = face_normals[f];
        new_normals[3 * f + 0]      = facenormal;
        new_normals[3 * f + 1]      = facenormal;
        new_normals[3 * f + 2]      = facenormal;
    }
    normals = std::move(new_normals);

    // Copy texcoords if necessary
    if (!texcoords.empty()) {
        std::vector<StVector2f> new_texcoords;
        new_texcoords.resize(faceCount() * 3);
        for (size_t f = 0; f < faceCount(); ++f) {
            new_texcoords[3 * f + 0] = texcoords[indices[4 * f + 0]];
            new_texcoords[3 * f + 1] = texcoords[indices[4 * f + 1]];
            new_texcoords[3 * f + 2] = texcoords[indices[4 * f + 2]];
        }
        texcoords = std::move(new_texcoords);
    }

    // Setup new indexing list
    for (size_t f = 0; f < faceCount(); ++f) {
        indices[4 * f + 0] = 3 * f + 0;
        indices[4 * f + 1] = 3 * f + 1;
        indices[4 * f + 2] = 3 * f + 2;
    }
}

bool TriMesh::isAPlane() const
{
    constexpr float PlaneEPS = 1e-5f;

    // A plane is given by two triangles
    if (faceCount() != 2)
        return false;

    // We only allow planes given by four points
    if (vertices.size() != 4) {
        // Further check, maybe the points are not handled as unique
        if (vertices.size() > 4 && vertices.size() <= 6) {
            std::vector<Vector3f> uniques;
            for (const auto& v : vertices) {
                bool found = false;
                for (const auto& u : uniques) {
                    found = v.isApprox(u, PlaneEPS);
                    if (found)
                        break;
                }
                if (!found)
                    uniques.push_back(v);
            }
            if (uniques.size() != 4)
                return false;
        } else {
            return false;
        }
    }

    // If the two triangles are facing differently, its not a plane
    if (!face_normals[0].isApprox(face_normals[1], PlaneEPS))
        return false;

    // Check each edge. The second triangle has to have the same edges (but order might be different)
    float e1 = (vertices[indices[0]] - vertices[indices[1]]).squaredNorm();
    float e2 = (vertices[indices[1]] - vertices[indices[2]]).squaredNorm();
    float e3 = (vertices[indices[2]] - vertices[indices[0]]).squaredNorm();
    float e4 = (vertices[indices[4 + 0]] - vertices[indices[4 + 1]]).squaredNorm();
    float e5 = (vertices[indices[4 + 1]] - vertices[indices[4 + 2]]).squaredNorm();
    float e6 = (vertices[indices[4 + 2]] - vertices[indices[4 + 0]]).squaredNorm();

    auto safeCheck = [](float a, float b) { return std::abs(a - b) <= PlaneEPS; };
    if (!safeCheck(e1, e4) && !safeCheck(e2, e4) && !safeCheck(e3, e4))
        return false;
    if (!safeCheck(e1, e5) && !safeCheck(e2, e5) && !safeCheck(e3, e5))
        return false;
    if (!safeCheck(e1, e6) && !safeCheck(e2, e6) && !safeCheck(e3, e6))
        return false;

    return true;
}

inline static void addTriangle(TriMesh& mesh, const Vector3f& origin, const Vector3f& xAxis, const Vector3f& yAxis, uint32 off)
{
    constexpr uint32 M = 0;
    const Vector3f lN  = xAxis.cross(yAxis);
    const float area   = lN.norm() / 2;
    const Vector3f N   = lN / area;

    mesh.vertices.insert(mesh.vertices.end(), { origin, origin + xAxis, origin + yAxis });
    mesh.normals.insert(mesh.normals.end(), { N, N, N });
    mesh.texcoords.insert(mesh.texcoords.end(), { Vector2f(0, 0), Vector2f(1, 0), Vector2f(0, 1) });
    mesh.face_normals.insert(mesh.face_normals.end(), N);
    mesh.face_inv_area.insert(mesh.face_inv_area.end(), 1 / area);
    mesh.indices.insert(mesh.indices.end(), { 0 + off, 1 + off, 2 + off, M });
}

inline static void addPlane(TriMesh& mesh, const Vector3f& origin, const Vector3f& xAxis, const Vector3f& yAxis, uint32 off)
{
    constexpr uint32 M = 0;
    const Vector3f lN  = xAxis.cross(yAxis);
    const float area   = lN.norm() / 2;
    const Vector3f N   = lN / area;

    mesh.vertices.insert(mesh.vertices.end(), { origin, origin + xAxis, origin + xAxis + yAxis, origin + yAxis });
    mesh.normals.insert(mesh.normals.end(), { N, N, N, N });
    mesh.texcoords.insert(mesh.texcoords.end(), { Vector2f(0, 0), Vector2f(1, 0), Vector2f(1, 1), Vector2f(0, 1) });
    mesh.face_normals.insert(mesh.face_normals.end(), { N, N });
    mesh.face_inv_area.insert(mesh.face_inv_area.end(), { 1 / area, 1 / area });
    mesh.indices.insert(mesh.indices.end(), { 0 + off, 1 + off, 2 + off, M, 0 + off, 2 + off, 3 + off, M });
}

inline static void addDisk(TriMesh& mesh, const Vector3f& origin, const Vector3f& N, const Vector3f& Nx, const Vector3f& Ny,
                           float radius, uint32 sections, uint32 off, bool fill_cap)
{
    constexpr uint32 M = 0;
    const float step   = 1.0f / sections;

    if (fill_cap) {
        mesh.vertices.insert(mesh.vertices.end(), origin);
        mesh.normals.insert(mesh.normals.end(), N);
        mesh.texcoords.insert(mesh.texcoords.end(), Vector2f(0, 0));
    }

    for (uint32 i = 0; i < sections; ++i) {
        const float x = std::cos(2 * Pi * step * i);
        const float y = std::sin(2 * Pi * step * i);

        mesh.vertices.insert(mesh.vertices.end(), { radius * Nx * x + radius * Ny * y + origin });
        mesh.normals.insert(mesh.normals.end(), N);
        mesh.texcoords.insert(mesh.texcoords.end(), Vector2f(0.5f * (x + 1), 0.5f * (y + 1)));
    }

    if (!fill_cap)
        return;

    const float area    = 2 * Pi * radius;
    const float secArea = area / sections;
    const uint32 start  = 1; // Skip disk origin
    for (uint32 i = 0; i < sections; ++i) {
        uint32 C  = i + start;
        uint32 NC = (i + 1 < sections ? i + 1 : 0) + start;
        mesh.face_normals.insert(mesh.face_normals.end(), N);
        mesh.face_inv_area.insert(mesh.face_inv_area.end(), 1 / secArea);
        mesh.indices.insert(mesh.indices.end(), { 0 + off, C + off, NC + off, M });
    }
}

TriMesh TriMesh::MakeUVSphere(const Vector3f& center, float radius, uint32 stacks, uint32 slices)
{
    constexpr uint32 M = 0;
    TriMesh mesh;

    const uint32 count = slices * stacks;
    mesh.vertices.reserve(count);
    mesh.normals.reserve(count);
    mesh.texcoords.reserve(count);

    float drho   = 3.141592f / (float)stacks;
    float dtheta = 2 * 3.141592f / (float)slices;

    const float area     = 4 * Pi * radius * radius / (stacks * slices); // TODO: Really?
    const float inv_area = 1 / area;

    // TODO: We create a 2*stacks of redundant vertices at the two critical points... remove them
    // Vertices
    for (uint32 i = 0; i <= stacks; ++i) {
        float rho  = (float)i * drho;
        float srho = (float)(sin(rho));
        float crho = (float)(cos(rho));

        for (uint32 j = 0; j < slices; ++j) {
            float theta  = (j == slices) ? 0.0f : j * dtheta;
            float stheta = (float)(-sin(theta));
            float ctheta = (float)(cos(theta));

            float x = stheta * srho;
            float y = ctheta * srho;
            float z = crho;

            const Vector3f N = Vector3f(x, y, z);

            mesh.vertices.push_back(N * radius + center);
            mesh.normals.push_back(N);
            mesh.texcoords.push_back(Vector2f(0.5 * theta / Pi, rho / Pi));

            mesh.face_normals.insert(mesh.face_normals.end(), { N, N });
            mesh.face_inv_area.insert(mesh.face_inv_area.end(), { inv_area, inv_area });
        }
    }

    // Indices
    for (uint32 i = 0; i <= stacks; ++i) {
        const uint32 currSliceOff = i * slices;
        const uint32 nextSliceOff = ((i + 1) % (stacks + 1)) * slices;

        for (uint32 j = 0; j < slices; ++j) {
            const uint32 nextJ = ((j + 1) % slices);
            const uint32 id0   = currSliceOff + j;
            const uint32 id1   = currSliceOff + nextJ;
            const uint32 id2   = nextSliceOff + j;
            const uint32 id3   = nextSliceOff + nextJ;

            mesh.indices.insert(mesh.indices.end(), { id2, id3, id1, M, id2, id1, id0, M });
        }
    }

    return mesh;
}

// Based on http://twistedoakstudios.com/blog/Post1080_my-bug-my-bad-1-fractal-spheres
TriMesh TriMesh::MakeIcoSphere(const Vector3f& center, float radius, uint32 subdivisions)
{
    constexpr uint32 M = 0;
    TriMesh mesh;

    // Create vertices (4 per axis plane)
    constexpr float GoldenRatio = 1.618033989f;
    for (int d = 0; d < 3; d++) {
        for (int s1 = -1; s1 <= +1; s1 += 2) {
            for (int s2 = -1; s2 <= +1; s2 += 2) {
                StVector3f vec   = StVector3f::Zero();
                vec[(d + 1) % 3] = GoldenRatio * s1;
                vec[(d + 2) % 3] = 1 * s2;
                mesh.vertices.push_back(vec.normalized());
            }
        }
    }

    // Create the triangles that have a point on each of the three axis planes
    const auto get_index = [](int d, int s1, int s2) { return d * 4 + (s1 + 1) + ((s2 + 1) >> 1); };
    for (int s1 = -1; s1 <= +1; s1 += 2) {
        for (int s2 = -1; s2 <= +1; s2 += 2) {
            for (int s3 = -1; s3 <= +1; s3 += 2) {
                int rev   = s1 * s2 * s3 == -1;
                uint32 i1 = (uint32)get_index(0, s1, s2);
                uint32 i2 = (uint32)get_index(1, s2, s3);
                uint32 i3 = (uint32)get_index(2, s3, s1);
                mesh.indices.insert(mesh.indices.end(), { i1, rev ? i3 : i2, rev ? i2 : i3, M });
            }
        }
    }

    // Create the triangles that have two points on one axis plane and one point on another
    for (int d = 0; d < 3; d++) {
        for (int s1 = -1; s1 <= +1; s1 += 2) {
            for (int s2 = -1; s2 <= +1; s2 += 2) {
                auto rev = s1 * s2 == +1;
                auto i2  = (uint32)get_index(d, s1, -1);
                auto i1  = (uint32)get_index(d, s1, +1);
                auto i3  = (uint32)get_index((d + 2) % 3, s2, s1);
                mesh.indices.insert(mesh.indices.end(), { i1, rev ? i3 : i2, rev ? i2 : i3, M });
            }
        }
    }

    // Refine
    for (uint32 i = 0; i < subdivisions; ++i) {
        // Place new vertices at centers of spherical edges between existing vertices
        std::unordered_map<uint32, uint32> edgeVertexMap;
        const size_t prev_size = mesh.vertices.size();
        for (auto tri = mesh.indices.begin(); tri != mesh.indices.end(); tri += 4) {
            for (auto j = 0; j < 3; j++) {
                uint32 i1 = tri[j];
                uint32 i2 = tri[(j + 1) % 3];
                if (i1 >= i2)
                    continue; // avoid adding the same edge vertex twice (once from X to Y and once from Y and X)
                uint32 undirectedEdgeId = i1 * prev_size + i2;

                edgeVertexMap[undirectedEdgeId] = mesh.vertices.size();
                mesh.vertices.push_back((mesh.vertices[i1] + mesh.vertices[i2]).normalized());
            }
        }

        // Create new triangles using old and new vertices
        std::vector<uint32> refinedIndices;
        for (auto tri = mesh.indices.begin(); tri != mesh.indices.end(); tri += 4) {
            // Find vertices at the center of each of the triangle's edges
            uint32 edgeCenterVertices[3];
            for (auto j = 0; j < 3; j++) {
                uint32 i1               = tri[j];
                uint32 i2               = tri[(j + 1) % 3];
                uint32 undirectedEdgeId = std::min(i1, i2) * prev_size + std::max(i1, i2);
                edgeCenterVertices[j]   = edgeVertexMap[undirectedEdgeId];
            }

            // Create a triangle covering the center, touching the three edges
            refinedIndices.insert(refinedIndices.end(), { edgeCenterVertices[0], edgeCenterVertices[1], edgeCenterVertices[2], M });
            // Create a triangle for each corner of the existing triangle
            for (auto j = 0; j < 3; j++)
                refinedIndices.insert(refinedIndices.end(), { tri[j], edgeCenterVertices[(j + 0) % 3], edgeCenterVertices[(j + 2) % 3], M });
        }

        mesh.indices = std::move(refinedIndices);
    }

    mesh.computeFaceNormals();
    mesh.computeVertexNormals();
    mesh.makeTexCoordsZero(); // TODO

    // Apply transformation
    for (auto& vertex : mesh.vertices)
        vertex = center + vertex * radius;

    return mesh;
}

TriMesh TriMesh::MakeDisk(const Vector3f& center, const Vector3f& normal, float radius, uint32 sections)
{
    sections = std::max<uint32>(3, sections);

    Vector3f Nx, Ny;
    Tangent::frame(normal, Nx, Ny);

    TriMesh mesh;
    addDisk(mesh, center, normal, Nx, Ny, radius, sections, 0, true);
    return mesh;
}

TriMesh TriMesh::MakePlane(const Vector3f& origin, const Vector3f& xAxis, const Vector3f& yAxis)
{
    TriMesh mesh;
    addPlane(mesh, origin, xAxis, yAxis, 0);
    return mesh;
}

TriMesh TriMesh::MakeTriangle(const Vector3f& p0, const Vector3f& p1, const Vector3f& p2)
{
    TriMesh mesh;
    addTriangle(mesh, p0, p1 - p0, p2 - p0, 0);
    return mesh;
}

TriMesh TriMesh::MakeRectangle(const Vector3f& p0, const Vector3f& p1, const Vector3f& p2, const Vector3f& p3)
{
    TriMesh mesh;
    addTriangle(mesh, p0, p1 - p0, p3 - p0, 0);
    addTriangle(mesh, p1, p2 - p1, p3 - p1, 3);
    return mesh;
}

TriMesh TriMesh::MakeBox(const Vector3f& origin, const Vector3f& xAxis, const Vector3f& yAxis, const Vector3f& zAxis)
{
    const Vector3f lll = origin;
    const Vector3f hhh = origin + xAxis + yAxis + zAxis;

    TriMesh mesh;
    addPlane(mesh, lll, yAxis, xAxis, 0);
    addPlane(mesh, lll, xAxis, zAxis, 4);
    addPlane(mesh, lll, zAxis, yAxis, 8);
    addPlane(mesh, hhh, -xAxis, -yAxis, 12);
    addPlane(mesh, hhh, -zAxis, -xAxis, 16);
    addPlane(mesh, hhh, -yAxis, -zAxis, 20);

    return mesh;
}

TriMesh TriMesh::MakeCone(const Vector3f& baseCenter, float baseRadius, const Vector3f& tipPos, uint32 sections, bool fill_cap)
{
    sections = std::max<uint32>(3, sections);

    const Vector3f H = (baseCenter - tipPos).normalized();
    Vector3f Nx, Ny;
    Tangent::frame(H, Nx, Ny);

    TriMesh mesh;
    addDisk(mesh, baseCenter, H, Nx, Ny, baseRadius, sections, 0, fill_cap);

    mesh.vertices.push_back(tipPos);
    mesh.normals.push_back(H);
    mesh.texcoords.push_back(Vector2f::Zero());

    const uint32 start = fill_cap ? 1 : 0; // Skip disk origin
    const uint32 tP    = mesh.vertices.size() - 1;
    for (uint32 i = 0; i < sections; ++i) {
        uint32 C  = i + start;
        uint32 NC = (i + 1 < sections ? i + 1 : 0) + start;

        const Vector3f dx = mesh.vertices[NC] - mesh.vertices[C];
        const Vector3f dy = tipPos - mesh.vertices[C];
        const Vector3f lN = -dx.cross(dy);
        const float area  = lN.norm() / 2;
        const Vector3f N  = lN / (2 * area);

        mesh.face_normals.insert(mesh.face_normals.end(), N);
        mesh.face_inv_area.insert(mesh.face_inv_area.end(), 1 / area);
        mesh.indices.insert(mesh.indices.end(), { C, tP, NC, 0 });
    }

    mesh.computeVertexNormals();
    return mesh;
}

TriMesh TriMesh::MakeCylinder(const Vector3f& baseCenter, float baseRadius, const Vector3f& topCenter, float topRadius, uint32 sections, bool fill_cap)
{
    sections = std::max<uint32>(3, sections);

    const Vector3f H = (baseCenter - topCenter).normalized();
    Vector3f Nx, Ny;
    Tangent::frame(H, Nx, Ny);

    TriMesh mesh;
    const uint32 off = fill_cap ? sections + 1 : sections;
    addDisk(mesh, baseCenter, H, Nx, Ny, baseRadius, sections, 0, fill_cap);
    addDisk(mesh, topCenter, -H, Nx, Ny, topRadius, sections, off, fill_cap);

    const uint32 start = fill_cap ? 1 : 0; // Skip disk origin
    for (uint32 i = 0; i < sections; ++i) {
        uint32 C  = i + start;
        uint32 NC = (i + 1 < sections ? i + 1 : 0) + start;

        const Vector3f dx = mesh.vertices[NC] - mesh.vertices[C];
        const Vector3f dy = mesh.vertices[C + off] - mesh.vertices[C];
        const Vector3f lN = -dx.cross(dy);
        const float area  = lN.norm() / 2;
        const Vector3f N  = lN / (2 * area);

        mesh.face_normals.insert(mesh.face_normals.end(), { N, N });
        mesh.face_inv_area.insert(mesh.face_inv_area.end(), { 1 / area, 1 / area });
        mesh.indices.insert(mesh.indices.end(), { C, C + off, NC, 0, C + off, NC + off, NC, 0 });
    }

    mesh.computeVertexNormals();
    return mesh;
}

} // namespace IG
