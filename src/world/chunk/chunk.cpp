#include "chunk.hpp"
#include "voxel.hpp"

#include "../../renderer/shader.hpp"
#include "../../renderer/camera.hpp"

#include "../block.hpp"

#include "../../util/fastnoise.hpp"

#include <random>

static std::default_random_engine generator;

Chunk::Chunk() {

}

Chunk::Chunk(int x, int z) {

	X = x, Z = z;

	Load();

}

Chunk::Chunk(int x, int z, std::vector<uint8_t> voxels) {

	X = x, Z = z;
	
	Voxels = voxels;
	
	Load();

}

Chunk::Chunk(int x, int z, std::shared_ptr<FastNoise> terrainGenerator) {

	X = x, Z = z;

	for (x = 0; x < CHUNK_WIDTH; x++)
	for (int y = 0; y < CHUNK_HEIGHT; y++)
	for (z = 0; z < CHUNK_DEPTH; z++) {

		if (pow(y / (float)CHUNK_HEIGHT, 1.1024f) + terrainGenerator->GetValueFractal(x + (Z * CHUNK_WIDTH), y, z + (X * CHUNK_DEPTH))  * 0.60f < 0.3f) {
	
			Voxels.push_back((uint8_t)EBlockType::Grass);
	
		} else {
	
			Voxels.push_back((uint8_t)EBlockType::Air);
	
		}

	}

	Load();

}

void Chunk::Load() {

	if (Loaded)
		return;

	m_model = glm::translate(glm::mat4(1.0f), { X * CHUNK_WIDTH, 0, Z * CHUNK_DEPTH });

	if (!Voxels.empty()) {
		m_mesh();
		Loaded = true;
		return;
	}

	// Generate a superflat chunk if nothing is there
	// [x + WIDTH * (y + HEIGHT * z)]
	for (int x = 0; x < CHUNK_WIDTH; x++)
	for (int y = 0; y < CHUNK_HEIGHT; y++)
	for (int z = 0; z < CHUNK_DEPTH; z++) {

		if (y > 32) {
			Voxels.push_back((uint8_t)EBlockType::Air);
			continue;
		}

		if (y == 0)
			Voxels.push_back((uint8_t)EBlockType::Bedrock);
		else if (y < 28)
			Voxels.push_back((uint8_t)EBlockType::Stone);
		else if (y < 32)
			Voxels.push_back((uint8_t)EBlockType::Dirt);
		else
			Voxels.push_back((uint8_t)EBlockType::Grass);

	}

	m_mesh();
	Loaded = true;

}

void Chunk::UploadMesh() {

	if (!MeshReady)
		return;

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	std::vector<glm::vec3> data;
	data.insert(data.end(), m_vertices.begin(), m_vertices.end());
	data.insert(data.end(), m_uvs.begin(), m_uvs.end());

	m_numVerts = m_vertices.size();

	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(glm::vec3), &data[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const void*)(m_vertices.size() * sizeof(glm::vec3)));

	m_vertices.clear();
	m_uvs.clear();

	data.clear();

	glBindVertexArray(0);

	MeshReady = !MeshReady;

}

void Chunk::Render(std::shared_ptr<Camera> camera, std::shared_ptr<Shader> shader) {

	if (!Loaded)
		return;

	shader->Use();
	glBindVertexArray(m_vao);

	GLint uniTrans = glGetUniformLocation(shader->Program, "model");
	glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(m_model));

	GLint uniView = glGetUniformLocation(shader->Program, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));

	GLint uniProj = glGetUniformLocation(shader->Program, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(camera->GetProjectionMatrix()));

	glDrawArrays(GL_TRIANGLES, 0, m_numVerts);

}

void Chunk::Update(std::vector<uint8_t> voxels) {

	Voxels = voxels;
	m_mesh();

}

uint8_t Chunk::BlockAt(int x, int y, int z) {

	if (x > CHUNK_WIDTH  - 1) return 0;
	if (y > CHUNK_HEIGHT - 1) return 0;
	if (z > CHUNK_DEPTH  - 1) return 0;

	if (x < 0) return 0;
	if (y < 0) return 0;
	if (z < 0) return 0;

	return Voxels[x + CHUNK_WIDTH * (y + CHUNK_HEIGHT * z)];

}

void Chunk::m_mesh() {

	// TODO: Use greedy meshing for MAXIMUM performance
	for (int x = 0; x < CHUNK_WIDTH; x++)
	for (int y = 0; y < CHUNK_HEIGHT; y++)
	for (int z = 0; z < CHUNK_DEPTH; z++) {

		std::vector<glm::vec3> tempVerts;
		std::vector<glm::vec3> tempUVs;

		uint8_t block = BlockAt(x, y, z);

		if (block == EBlockType::Air) continue;

		Voxel tmp({x, y, z}, block);

		if (BlockAt(x + 1, y, z) == EBlockType::Air)
			tmp.AddFace(EFaceType::Right);

		if (BlockAt(x - 1, y, z) == EBlockType::Air)
			tmp.AddFace(EFaceType::Left);

		if (BlockAt(x, y + 1, z) == EBlockType::Air)
			tmp.AddFace(EFaceType::Top);

		if (BlockAt(x, y - 1, z) == EBlockType::Air)
			tmp.AddFace(EFaceType::Bottom);

		if (BlockAt(x, y, z + 1) == EBlockType::Air)
			tmp.AddFace(EFaceType::Front);

		if (BlockAt(x, y, z - 1) == EBlockType::Air)
			tmp.AddFace(EFaceType::Back);
		
		tmp.GetMesh(tempVerts, tempUVs);

		m_vertices.insert(m_vertices.end(), tempVerts.begin(), tempVerts.end());
		m_uvs.insert(m_uvs.end(), tempUVs.begin(), tempUVs.end());

		tmp.Clear();

	}

	MeshReady = true;

}

Chunk::~Chunk() {

}
