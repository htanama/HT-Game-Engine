#pragma once
#include <glad/glad.h>

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define MAX_BONE_INFLUENCE 4

struct Vertex {
   // position
   glm::vec3 Position;
   // normal
   glm::vec3 Normal;
   // texCoords
   glm::vec2 TexCoords;
   // tangent
   glm::vec3 Tangent;
   // bitangent
   glm::vec3 Bitangent;
	//bone indexes which will influence this vertex
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	//weights from each bone
	float m_Weights[MAX_BONE_INFLUENCE];

};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    unsigned int Vao, Vbo, Ebo;

    // mesh data
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;


   mesh(const std::vector<vertex>& vertices,
       const std::vector<unsigned int>& indices,
       const std::vector<texture>& textures)
   {
       this->vertices = vertices;
       this->indices = indices;
       this->textures = textures;

       // now that we have all the required data, set the vertex buffers and its attribute pointers.
       setupmesh();
   }

   // initializes all the buffer objects/arrays
   void setupmesh()
   {
       // safety check: if the mesh is empty, stop here to prevent crash
       if (vertices.empty()) return;

       // create buffers/arrays       
       glgenbuffers(1, &vbo);
       glgenbuffers(1, &ebo);
       glgenvertexarrays(1, &vao);

       // debug: check if opengl actually gave us valid ids
       if (vbo == 0 || ebo == 0 || vao == 0) {            
           logger::log("your opengl context might be invalid.");
           return;
       }

       glbindvertexarray(vao);
       // load data into vertex buffers
       glbindbuffer(gl_array_buffer, vbo);
       // a great thing about structs is that their memory layout is sequential for all its items.
       // the effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
       // again translates to 3/2 floats which translates to a byte array.
       glbufferdata(gl_array_buffer, vertices.size() * sizeof(vertex), &vertices[0], gl_static_draw);  

       if (!indices.empty()) {
           glbindbuffer(gl_element_array_buffer, ebo);
           glbufferdata(gl_element_array_buffer, indices.size() * sizeof(unsigned int), &indices[0], gl_static_draw);
       }

       // set the vertex attribute pointers
       // vertex positions
       glenablevertexattribarray(0);	
       glvertexattribpointer(0, 3, gl_float, gl_false, sizeof(vertex), (void*)0);
       // vertex normals
       glenablevertexattribarray(1);	
       glvertexattribpointer(1, 3, gl_float, gl_false, sizeof(vertex), (void*)offsetof(vertex, normal));
       // vertex texture coords
       glenablevertexattribarray(2);	
       glvertexattribpointer(2, 2, gl_float, gl_false, sizeof(vertex), (void*)offsetof(vertex, texcoords));
       // vertex tangent
       glenablevertexattribarray(3);
       glvertexattribpointer(3, 3, gl_float, gl_false, sizeof(vertex), (void*)offsetof(vertex, tangent));
       // vertex bitangent
       glenablevertexattribarray(4);
       glvertexattribpointer(4, 3, gl_float, gl_false, sizeof(vertex), (void*)offsetof(vertex, bitangent));
		// ids
		glenablevertexattribarray(5);
		glvertexattribipointer(5, 4, gl_int, sizeof(vertex), (void*)offsetof(vertex, m_boneids));

		// weights
		glenablevertexattribarray(6);
		glvertexattribpointer(6, 4, gl_float, gl_false, sizeof(vertex), (void*)offsetof(vertex, m_weights));
       glbindvertexarray(0);
   }

	
    ~Mesh() {
        glDeleteVertexArrays(1, &Vao);
        glDeleteBuffers(1, &Vbo);
        glDeleteBuffers(1, &Ebo);
    }
};
