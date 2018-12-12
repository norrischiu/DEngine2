import os.path
import sys
import fbx
from fbx import FbxLayerElement
from fbx import FbxSurfaceMaterial
from fbx import FbxLayeredTexture
from fbx import FbxTexture
from fbx import FbxPropertyString

class ControlPoint:
	def __init__(self):
		self.skinWeight = [];
		self.jointIndex = [];

class VertexData:
	def __init__(self):
		self.pos = [];
		self.index = [];
		self.index = -1;
		self.norm = [];
		self.tang = [];
		self.texCoord = [];
		self.skinWeight = [];
		self.jointIndex = [];
	def __eq__(self, other):
		return self.pos == other.pos

def StoreMeshToFile(scene, meshName):
	
	package = meshName
	assetsOut = os.path.join(sys.path[0], "Export", meshName)
	vertices = []
	indices = []
	
	childNum = scene.GetRootNode().GetChildCount()
	print("Total child num: %i"%childNum)
	
	for n in range(0, childNum):
		print("Processing child %i"%n)
		node = scene.GetRootNode().GetChild(n)
		print("Child name: %s"%node.GetName())
		mesh = node.GetMesh()
		if (mesh is None):
			continue
		triNum = mesh.GetPolygonCount()
		
		positions = mesh.GetControlPoints()
		vertexNum = mesh.GetControlPointsCount()
		indexNum = mesh.GetPolygonVertexCount()
		skinDeformer = mesh.GetDeformer(0)
		if (skinDeformer is not None):
			skinType = skinDeformer.GetSkinningType()
			clusterNum = skinDeformer.GetClusterCount();
		
		layer = mesh.GetLayer(0)
		if (layer.GetNormals() == None):
			mesh.GenerateNormals(True)
			print("Generating normals")
		if (layer.GetTangents() == None):
			mesh.GenerateTangentsDataForAllUVSets(True)
			print("Generating tangents")
		normals = layer.GetNormals().GetDirectArray()
		normalMode = (layer.GetNormals().GetMappingMode(), layer.GetNormals().GetReferenceMode())
		tangents = layer.GetTangents().GetDirectArray()
		tangentMode = (layer.GetTangents().GetMappingMode()), layer.GetTangents().GetReferenceMode()
		binormals = layer.GetBinormals().GetDirectArray()
		UVs = layer.GetUVs().GetDirectArray()
		UVMode = (layer.GetUVs().GetMappingMode(), layer.GetUVs().GetReferenceMode())
		material = node.GetMaterial(0)
		
		print("num of vertex: %i"%len(positions))
		print("num of index: %i"%indexNum)
		print("num of polygon: %i"%triNum)
		
		print("Normal modes: %i %i"%(normalMode[0], normalMode[1]))
		print("Tangent modes: %i %i"%(tangentMode[0], tangentMode[1]))
		print("UV modes: %i %i"%(UVMode[0], UVMode[1]))
		
		c = 0
		for t in layer.GetUVs().GetDirectArray():
			c = c + 1
		print("normal size: %i"%c)
		
		# Construct data
		
		if (skinDeformer is not None):	
			controlPoints = []
			for i in range(0, vertexNum):
				controlPoints.append(ControlPoint())
			
			if (skinType == 0):
				print("Using 1 to 1 skinning")
			if (skinType == 1):
				print("Using 1 to 4 skinning")
			# Cluster refers to joint in this context
			for i in range(0, clusterNum):
				currCluster = skinDeformer.GetCluster(i)
				# Get skin weight
				clusterVertexNum = currCluster.GetControlPointIndicesCount()
				for j in range(0, clusterVertexNum):
					index = currCluster.GetControlPointIndices()[j]
					currWeight = currCluster.GetControlPointWeights()[j]
					controlPoints[index].skinWeight.append(currWeight)
					controlPoints[index].jointIndex.append(i)
									
			# Fix skinning to 4 joints
			for cp in controlPoints:
				while (len(cp.skinWeight) < 4):
					cp.jointIndex.append(0)
					cp.skinWeight.append(0.0)
				while (len(cp.skinWeight) > 4):
					index = cp.skinWeight.index(min(cp.skinWeight))
					del cp.skinWeight[index]
					del cp.jointIndex[index]
					cp.skinWeight = [i/sum(cp.skinWeight) for i in cp.skinWeight]	
					
		currNodeIndices = mesh.GetPolygonVertices()
		#indices = indices + currNodeIndices Merge all to one mesh
		indices = currNodeIndices
		
		currNodeVertices = []
		for i in range(0, triNum):
			for j in range(0, 3):
				v = mesh.GetPolygonVertex(i, j) # index
				# Position
				currNodeVertices.append(VertexData())
				currNodeVertices[3*i + j].pos.append(positions[v][0])
				currNodeVertices[3*i + j].pos.append(positions[v][1])
				currNodeVertices[3*i + j].pos.append(positions[v][2])
				# Normal
				if normalMode[0] == 2: # eByPolygonVertex 
					index = 3*i + j
				else:
					index = v
				if normalMode[1] == 2: # eIndexToDirect
					index = layer.GetNormals().GetIndexArray().GetAt(index)
				currNodeVertices[3*i + j].norm.append(normals[index][0])
				currNodeVertices[3*i + j].norm.append(normals[index][1])
				currNodeVertices[3*i + j].norm.append(normals[index][2])
				# Tangent
				if tangentMode[0] == 2: # eByPolygonVertex 
					index = 3*i + j
				else:
					index = v
				if tangentMode[1] == 2: # eIndexToDirect
					index = layer.GetTangents().GetIndexArray().GetAt(index)
				currNodeVertices[3*i + j].tang.append(tangents[3*i + j][0])
				currNodeVertices[3*i + j].tang.append(tangents[3*i + j][1])
				currNodeVertices[3*i + j].tang.append(tangents[3*i + j][1])
				# Texture Coordinate
				if UVMode[0] == 2: # eByPolygonVertex 
					index = 3*i + j
				else:
					index = v
				if UVMode[1] == 2: # eIndexToDirect
					index = layer.GetUVs().GetIndexArray().GetAt(index)
				#index = mesh.GetTextureUVIndex(i, j)
				currNodeVertices[3*i + j].texCoord.append(UVs[index][0])
				currNodeVertices[3*i + j].texCoord.append(UVs[index][1])
				# Skin Weight
				if (skinDeformer is not None):
					currNodeVertices[3*i + j].skinWeight = controlPoints[v].skinWeight
					currNodeVertices[3*i + j].jointIndex = controlPoints[v].jointIndex
					
		#vertices = vertices + currNodeVertices
		vertices = currNodeVertices
		
		# Write position
		positionBufferFileName = '%s%i_vertex.bufa' % (meshName, n)
		
		dirPath = os.path.join(assetsOut, "PositionBuffers")
		if not os.path.exists(dirPath):
			print("Creating %s folder" % dirPath)
			os.mkdir(dirPath)

		pbf = open(os.path.join(dirPath, positionBufferFileName), 'w')
		pbf.write("POSITION_BUFFER_V1\n%d\n"%(len(vertices)))
		for v in vertices:
			pbf.write("%f %f %f\n"%(-v.pos[0] / 100.0, v.pos[1] / 100.0, v.pos[2] / 100.0)) # flip z axis
			
		pbf.close()
		
		# Write index
		indexBufferFileName = '%s%i_index.bufa' % (meshName, n)
		
		dirPath = os.path.join(assetsOut, "IndexBuffers")
		if not os.path.exists(dirPath):
			print("Creating %s folder" % dirPath)
			os.mkdir(dirPath)

		ibf = open(os.path.join(dirPath, indexBufferFileName), 'w')
		ibf.write("INDEX_BUFFER_V1\n3\n%d\n"%(triNum))
		for i in range(0, triNum):
			#ibf.write("%i "%(indices[i]))
			ibf.write("%i %i %i\n"%(i*3 + 0, i*3 + 2, i*3 + 1))
			#if ((i+1)%3 == 0):
			#	ibf.write("\n")
			
		ibf.close()
		
		# Write normals
		normalBufferFileName = '%s%i_normal.bufa' % (meshName, n)

		dirPath = os.path.join(assetsOut, "NormalBuffers")
		if not os.path.exists(dirPath):
			print("Creating %s folder" % dirPath)
			os.mkdir(dirPath)

		nbf = open(os.path.join(dirPath, normalBufferFileName), 'w')
		nbf.write("NORMAL_BUFFER\n%d\n"%(len(vertices)))
		for v in vertices:
			nbf.write("%f %f %f\n"%(-v.norm[0], v.norm[1], v.norm[2])) # flip z axis
			
		nbf.close()
		
		# Write tangents
		tangentBufferFileName = '%s%i_tangent.bufa' % (meshName, n)

		dirPath = os.path.join(assetsOut, "TangentBuffers")
		if not os.path.exists(dirPath):
			print("Creating %s folder" % dirPath)
			os.mkdir(dirPath)

		tbf = open(os.path.join(dirPath, tangentBufferFileName), 'w')
		tbf.write("TANGENT_BUFFER\n%d\n"%(len(vertices)))
		for v in vertices:
			tbf.write("%f %f %f\n"%(-v.tang[0], v.tang[1], v.tang[2])) # flip z axis
			
		tbf.close()
		
		# Write texture coordinates
		texCoordBufferFileNames = '%s%i_texture.bufa'%(meshName, n)
		
		dirPath = os.path.join(assetsOut, "TexCoordBuffers")
		if not os.path.exists(dirPath):
			print("Creating %s folder" % dirPath)
			os.mkdir(dirPath)
		
		tcbf = open(os.path.join(dirPath, texCoordBufferFileNames), 'w')
		tcbf.write("TEX_COORD_BUFFER\n%d\n"%(len(vertices)))
		for v in vertices:
			if v.texCoord != None:	
				'''while (v.texCoord[0] < 0.0):
					v.texCoord[0] = v.texCoord[0] * -1.0
				while (v.texCoord[1] < 0.0):
					v.texCoord[1] = v.texCoord[1] * -1.0	
				while (v.texCoord[0] > 1.0):
					v.texCoord[0] = v.texCoord[0] - 1.0
				while (v.texCoord[1] > 1.0):
					v.texCoord[1] = v.texCoord[1] - 1.0'''
				tcbf.write("%f %f\n"%(v.texCoord[0], 1.0 - v.texCoord[1])) #flip v(y) component of tex coord
			else:
				tcbf.write("0 0\n") #dont allow no texture buffer.
		tcbf.close()
		
		# Write material
		materialFileName = '%s%i_material.mate' % (meshName, n)

		dirPath = os.path.join(assetsOut, "Material")
		if not os.path.exists(dirPath):
			print("Creating %s folder" % dirPath)
			os.mkdir(dirPath)

		mf = open(os.path.join(dirPath, materialFileName), 'w')
		mf.write("MATERIAL\n")
		#emissive
		mf.write("%f %f %f\n"%(material.Emissive.Get()[0], material.Emissive.Get()[1], material.Emissive.Get()[2])) 
		#diffuse
		mf.write("%f %f %f\n"%(material.Diffuse.Get()[0], material.Diffuse.Get()[1], material.Diffuse.Get()[2])) 
		#specular
		#shininess
		if material.ClassId != fbx.FbxSurfaceLambert.ClassId:
			mf.write("%f %f %f\n"%(material.Specular.Get()[0], material.Specular.Get()[1], material.Specular.Get()[2])) 
			mf.write("%f\n"%material.Shininess.Get())
		else:
			mf.write("0 0 0\n")
			mf.write("1\n")
		#texture
		textureList = []
		for propertyIndex in range(0, fbx.FbxLayerElement.sTypeTextureCount()):
			property = material.FindProperty(fbx.FbxLayerElement.sTextureChannelNames(propertyIndex))
			for i in range(0, property.GetSrcObjectCount(fbx.FbxFileTexture.ClassId)):
				texture = property.GetSrcObject(fbx.FbxFileTexture.ClassId, i)
				if (property.GetName() != "AmbientColor"):
					path_list = texture.GetFileName().split(os.sep)
					filename = path_list[-1]
					filename = filename[8:]
					print(filename)
					filename = filename[:-4] + ".dds"
					textureList.append(property.GetName())
					textureList.append(filename)
					if (filename == "spnza_bricks_a_bump.dds"):
						textureList.append("Specular")
						textureList.append("spnza_bricks_a_spec.dds")
					if (filename == "sponza_arch_bump.dds"):
						textureList.append("Specular")
						textureList.append("sponza_arch_spec.dds")
					if (filename == "sponza_ceiling_a_diff.dds"):
						textureList.append("Specular")
						textureList.append("sponza_ceiling_a_spec.dds")
					if (filename == "sponza_column_a_bump.dds"):
						textureList.append("Specular")
						textureList.append("sponza_column_a_spec.dds")
					if (filename == "sponza_column_b_bump.dds"):
						textureList.append("Specular")
						textureList.append("sponza_column_b_spec.dds")
					if (filename == "sponza_column_c_bump.dds"):
						textureList.append("Specular")
						textureList.append("sponza_column_c_spec.dds")
					if (filename == "sponza_flagpole_diff.dds"):
						textureList.append("Specular")
						textureList.append("sponza_flagpole_spec.dds")
					if (filename == "sponza_floor_a_diff.dds"):
						textureList.append("Specular")
						textureList.append("sponza_floor_a_spec.dds")
					if (filename == "sponza_thorn_bump.dds"):
						textureList.append("Specular")
						textureList.append("sponza_thorn_spec.dds")
					if (filename == "vase_plant.dds"):
						textureList.append("Specular")
						textureList.append("vase_plant_spec.dds")
					if (filename == "vase_round_bump.dds"):
						textureList.append("Specular")
						textureList.append("vase_round_spec.dds")
				
		mf.write("%i\n"%(len(textureList)/2))
		print(len(textureList))
		for i in range(0, len(textureList)):
			mf.write("%s\n"%textureList[i])
		
		mf.close()

		# Write skin weight
		if (skinDeformer is not None):
			SkinWeightFileNames = '%s%i_weight.bufa'%(meshName, n)
			
			dirPath = os.path.join(assetsOut, "SkinWeight")
			if not os.path.exists(dirPath):
				print("Creating %s folder" % dirPath)
				os.mkdir(dirPath)
			
			swbf = open(os.path.join(dirPath, SkinWeightFileNames), 'w')
			swbf.write("SKIN_WEIGHT\n%d\n"%(len(vertices)))
			for v in vertices:
				for i in range(0, 4):
					swbf.write("%i %f\n"%(v.jointIndex[i], v.skinWeight[i]))
			swbf.close()