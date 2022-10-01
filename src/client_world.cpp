#pragma once
#define _USE_MATH_DEFINES

#include <cmath>
#include <vector>
#include <array>
#include <unordered_map>

#include "global.cpp"
#include "render.cpp"

std::array<int, 2> world_size_chunks = {2450, 1225};
std::array<int, 2> world_size = {world_size_chunks[0] * 16, world_size_chunks[1] * 16};

enum tile_ID{VOIDTILE, SAND, GRASS, WATER, TALL_GRASS};

struct Tile {
    tile_ID id;
    glm::vec2 texture;

    Tile() = default;
    Tile(tile_ID id, int texture) {
        this->id = id;
        this->texture = {(texture % 8) * 2, (texture / 8) * 2};
    }
};

struct Server_chunk {
    std::array<Tile, 512> tiles;
    glm::ivec2 chunk_index;
};

struct Chunk_data {
    GLuint vertex_array;
    GLuint array_buffer;
    GLuint element_buffer;

    std::vector<Vertex> vertex_vector;
    std::vector<uint32_t> index_vector;
    std::array<Tile, 512> tiles;
    glm::ivec2 chunk_index;

    int index_count;
    glm::mat4 transform_mat = glm::identity<glm::mat4>();

    int mesh_generated = 0;
    bool vertices_loaded = false;

    Chunk_data() = default;
    Chunk_data(Server_chunk& chunk) {
        this->tiles = chunk.tiles;
        this->chunk_index = chunk.chunk_index;
        transform_mat = glm::identity<glm::mat4>();//glm::translate(transform_mat, glm::vec3(chunk_index * 16, 0.0f));
    }
    
    void generate_mesh() {
        mesh_generated = 1;

        vertex_vector.clear();
        index_vector.clear();

        int counter = 0;
        for(int i = 0; i < 256; ++i) {
            Tile& current_tile = tiles[i];
            for(Vertex v : floor_vertices) {
                v.tex += current_tile.texture;
                v.pos += glm::vec3(chunk_index * 16, 0.0f) + glm::vec3(i % 16, i / 16, 0.0f);
                vertex_vector.push_back(v);
            }
            index_vector.push_back(counter);
            index_vector.push_back(counter + 1);
            index_vector.push_back(counter + 2);
            index_vector.push_back(counter + 2);
            index_vector.push_back(counter + 1);
            index_vector.push_back(counter + 3);

            counter += 4;
        }
        /*for(int i = 256; i < 512; ++i) {
            Tile& current_tile = tiles[i];
            if(current_tile.id != VOIDTILE) {
                for(Vertex v : object_vertices) {
                    v.tex += current_tile.texture;
                    v.pos += glm::vec3(chunk_index * 16, 0.0f) + glm::vec3((i - 256) % 16, (i - 256) / 16, 0.0f);
                    vertex_vector.push_back(v);
                }
                index_vector.push_back(counter);
                index_vector.push_back(counter + 1);
                index_vector.push_back(counter + 2);
                index_vector.push_back(counter + 2);
                index_vector.push_back(counter + 1);
                index_vector.push_back(counter + 3);

                counter += 4;
            }
        }*/

        index_count = index_vector.size();
        mesh_generated = 2;
    }

    void load_vertices() {
        //glDeleteBuffers(1, &array_buffer);
        //glDeleteBuffers(1, &element_buffer);
        //glDeleteVertexArrays(1, &vertex_array);

        // create vertex array
        glCreateVertexArrays(1, &vertex_array);
        glBindVertexArray(vertex_array);

        // create and bind buffers
        glCreateBuffers(1, &array_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, array_buffer);

        glCreateBuffers(1, &element_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);

        // load buffers with data
        glBufferData(GL_ARRAY_BUFFER, vertex_vector.size() * sizeof(Vertex), vertex_vector.data(), GL_STATIC_DRAW); // array buffer (vertices)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
        glEnableVertexAttribArray(1);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_vector.size() * sizeof(uint32_t), index_vector.data(), GL_STATIC_DRAW); // element buffer (indices)

        vertex_vector.clear();
        index_vector.clear();

        vertices_loaded = true;
    }

    void delete_buffers() {
        glDeleteBuffers(1, &array_buffer);
        glDeleteBuffers(1, &element_buffer);
        glDeleteVertexArrays(1, &vertex_array);
    }

    void render(GLuint shader_program, GLuint texture, glm::mat4& pv_mat) {
        glBindVertexArray(vertex_array);

        glUseProgram(shader_program);
        glBindTexture(GL_TEXTURE_2D, texture);        
        glUniformMatrix4fv(0, 1, GL_FALSE, &pv_mat[0][0]);
        glUniformMatrix4fv(1, 1, GL_FALSE, &transform_mat[0][0]);
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    }
};

tile_ID get_tile(std::unordered_map<unsigned int, Chunk_data>& loaded_chunks, unsigned int chunk_key, int x, int y) {
    if(loaded_chunks.contains(chunk_key)) {
        return loaded_chunks[chunk_key].tiles[x + y * 16].id;
    }
    return VOIDTILE;
}

int get_texture_variation(tile_ID target, std::array<tile_ID, 4> neighbors) {
    if(neighbors[0] == target) {
        if(neighbors[1] == target) {
            if(neighbors[2] == target) {
                if(neighbors[3] == target) return 15;
                return 13;
            } else if(neighbors[3] == target) return 14;
            return 9;
        } else if(neighbors[2] == target) {
            if(neighbors[3] == target) return 12;
            return 8;
        } else if(neighbors[3] == target) return 7;
        return 2;
    } else if(neighbors[1] == target) {
        if(neighbors[2] == target) {
            if(neighbors[3] == target) return 11;
            return 6;
        } else if(neighbors[3] == target) return 5;
        return 1;
    } else if(neighbors[2] == target) {
        if(neighbors[3] == target) return 10;
        return 4;
    } else if(neighbors[3] == target) return 3;
    return 0;
}