CPU-GPU data exchange
=====================


Vertex Attributes
-----------------


    Table 1. Vertex attribute locations in shader


| Loc | Attribute  | Condition of presence |
| --- | ---------- |                       |
|  0  | mVertex    |                       |
|  1  | mTexCoord  |                       |
|  2  | mNormal    |                       |
|  3  | mTangent   | if `m_Matopt & 2`     |
|  4  | mBitangent | if `m_Matopt & 2`     |


    Table 2. Basic vertex attribute format


| Offset | Attribute  |
| ------ | ---------- |
| 0      | Position X |
| 1      | Position Y |
| 2      | Position Z |
| 3      | Texture U  |
| 4      | Texture V  |
| 5      | Normal X   |
| 6      | Normal Y   |
| 7      | Normal Z   |


    Table 3. Tangent vertex attribute format


| Offset | Attribute   |
| ------ | ----------- |
| 0      | Tangent X   |
| 1      | Tangent Y   |
| 2      | Tangent Z   |
| 3      | Bitangent X |
| 4      | Bitangent Y |
| 5      | Bitangent Z |


    Table 4. Vertex attribute mapping


| Loc | Attribute  | Buffer number | Offset (floats)
| --- | ---------- | ------------- | ---------------
|  0  | mVertex    | 0             | 0
|  1  | mTexCoord  | 0             | 3
|  2  | mNormal    | 0             | 5
|  3  | mTangent   | 1             | 0
|  4  | mBitangent | 1             | 3


Uniform attributes
------------------


    Table 5. Scene-specific uniforms


| Name               |
| ------------------ |
| mCameraPosition    |
| mCameraDestination |
| mCameraMatrix      |
| mProjectionMatrix  |


    Table 6. Material-specific uniforms


| Name       | Description        |
| ---------- | ------------------ |
| m_Ka       | Ambient parameter  |
| m_Kd       | Diffuse color      |
| m_Ks       | Specular parameter |
| m_Ns       | Shininess          |
| m_map_Kd   | Diffuse map        |
| m_map_Bump | Bump map           |
| m_Matopt   | Material options   |


    Table 7. Mesh-specific uniforms


| Name         |
| ------------ |
| mModelMatrix |
