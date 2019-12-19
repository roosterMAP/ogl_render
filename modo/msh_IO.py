'''
FileType Description:
	First line is header ( start with 'h' character ) which contains the path to the source lxo and the number of surfaces. Spaces in path name are repaced with '%32' substring.
	Next is a list of surfaces (start with 's' character). The identifier char is followed by the number of unique verts then the number of polys.
		Each surface has a list of verts ( start with 'v' character ) followed by list of poly ( start with 'p' character )
		Verts are a list of floats: v pos.x pos.y pos.z norm.x norm.y norm.z uv.x uv.y
		Polys are a list of indexes of the previous list of vertices.

Example:
	h C:/test/test.lxo 2

	s MatName1 3 1 {
		v 0.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0
		v 1.0 0.0 0.0 0.0 1.0 1.0 0.0 0.0
		v 0.0 1.0 0.0 0.0 1.0 0.0 1.0 0.0
		p 0 1 2
	}
	s MatName1 4 2 {
		v 0.0 0.0 0.0 0.0 1.0 0.0 0.0 0.0
		v 1.0 0.0 0.0 0.0 1.0 1.0 0.0 0.0
		v 0.0 1.0 0.0 0.0 1.0 0.0 1.0 0.0
		v 1.0 1.0 0.0 0.0 1.0 1.0 1.0 0.0
		p 0 1 2
		p 1 3 2
	}
'''

import lx, lxu, lxifc
import os, sys, math

SERVER_NAME = 'timmy.MSH_IO'

persist_data = None

UNITS = [	( 'mm', 'Millimeters', 0.001 ),
			( 'cm', 'Centimeters', 0.01 ),
			( 'm', 'Meters', 1.0 ),
			( 'in', 'Inches', 0.0254 ) ]

EPSILON = 16 * sys.float_info.epsilon
def almost_equal( a, b, epsilon=EPSILON ):
	return( abs( a - b ) < epsilon )

def errorDialog( msg ):
	lx.eval( 'dialog.setup error' )
	lx.eval( 'dialog.title {ERROR}' )
	lx.eval( 'dialog.msg {{{}}}'.format( msg ) )
	lx.eval( 'dialog.result ok' )
	lx.eval( 'dialog.open' )

class Vert():
	def __init__( self, pos, norm, uv ):
		self._pos = pos
		self._norm = norm
		self._uv = uv

	def __repr__( self ):
		return str( '{} {} {} {} {} {} {} {}'.format(	round( self._pos[0], 10 ), round( self._pos[1], 10 ), round( self._pos[2], 10 ),
														round( self._norm[0], 10 ), round( self._norm[1], 10 ), round( self._norm[2], 10 ),
														round( self._uv[0], 10 ), round( self._uv[1], 10 ) ) )

class Surface():
	def __init__( self, name, vertList, polyList ):
		self.name = name
		self.verts = vertList
		self.polys = polyList
		self.vIDList = [None] * len( vertList )

class _getMatPTags( lxifc.Visitor ):
	def __init__( self, polyAccessor ):
		self.polyAccessor = polyAccessor
		self.ptags = set()

	def vis_Evaluate( self ):
		stringTag = lx.object.StringTag( self.polyAccessor )
		ptag = stringTag.Get( lx.symbol.i_POLYTAG_MATERIAL )
		self.ptags.add( ptag )

def getChildren( parent, itype=None, recursive=False, children=None ):
	scn_svc = lx.service.Scene()
	
	if children is None:
		children = set()

	for i in range( parent.SubCount() ):
		child = parent.SubByIndex( i )

		if( itype is None or itype == scn_svc.ItemTypeName( child.Type() ) ):
			children.add( child )
			
		if( recursive ):
			children |= getChildren( child, itype, recursive, children )
			
	return children

class _surfaceVisitor( lxifc.Visitor ):
	def __init__( self, mesh, polyAccessor, item, uvMapID, nmlMapID, checkedMark ):
		self.item = item
		self.mesh = mesh
		self.polyAccessor = polyAccessor
		self.surfaceNameList = []
		self.surfaceList = []
		self.vertAccessor = lx.object.Point( mesh.PointAccessor() )
		self.uvMapID = uvMapID
		self.nmlMapID = nmlMapID
		self.uvStorage = lx.object.storage( 'f', 2 )
		self.nmlStorage = lx.object.storage( 'f', 3 )
		self.checkedMark = checkedMark

		self.smAngleDict = {}
		self.initSMAngles()

	def initSMAngles( self ):
		scn_svc = lx.service.Scene()
		scene = self.item.Context()
		chanRead = scene.Channels( None, lx.service.Selection().GetTime() )

		#set default smoothing angle for all ptags
		for i in range( self.mesh.PTagCount( lx.symbol.i_POLYTAG_MATERIAL ) ):
			ptag = self.mesh.PTagByIndex( lx.symbol.i_POLYTAG_MATERIAL, i )
			self.smAngleDict[ptag] = 40.0
		
		#fetch smoothing angle from advaned material within mask items with associated ptags
		typeID = scn_svc.ItemTypeLookup( lx.symbol.sITYPE_MASK )
		for i in range( scene.ItemCount( typeID ) ):
			maskItem = scene.ItemByIndex( typeID, i )
			
			#get ptag for mask tiem
			chanIndex = maskItem.ChannelLookup( lx.symbol.sICHAN_MASK_PTAG )
			maskPTag = chanRead.Value( maskItem, chanIndex )

			#get smoothing angle from material item within mask
			for child in getChildren( parent=maskItem, itype=lx.symbol.sITYPE_ADVANCEDMATERIAL, recursive=True ):
				chanIndex = child.ChannelLookup( lx.symbol.sICHAN_ADVANCEDMATERIAL_SMANGLE )
				smAngle = chanRead.Value( child, chanIndex )
				self.smAngleDict[maskPTag] = math.degrees( smAngle )
				break

	def vis_Evaluate( self ):
		innerSet = set() #Polygons whose connected polygons have been added to the outer set.
		outerSet = [] #Polygons whose connected polygons are yet to be checked for connectivity.

		outerSet.append( self.polyAccessor.ID() ) #add current poly for outer set to be checked
		while len( outerSet ) > 0:
			pID = outerSet[0]
			self.polyAccessor.Select( pID )

			#Add polygon to inner list, remove from outer list, and mark as checked
			innerSet.add( pID )
			outerSet.remove( pID )
			self.polyAccessor.SetMarks( self.checkedMark )

			#get material name and smoothing group
			stringTag = lx.object.StringTag( self.polyAccessor )
			matPTag = ''
			try: matPTag = stringTag.Get( lx.symbol.i_POLYTAG_MATERIAL )
			except LookupError: pass
			smthPTag = ''
			try: smthPTag = stringTag.Get( lx.symbol.i_POLYTAG_SMOOTHING_GROUP )
			except LookupError: pass

			polyNormal = self.polyAccessor.Normal()

			#find neigh polys connected to current poly via uvs
			for i in range( self.polyAccessor.VertexCount() ):
				vID = self.polyAccessor.VertexByIndex( i )

				#get uvVal
				uvValue = None
				if( self.uvMapID is not None and self.polyAccessor.MapEvaluate( self.uvMapID, vID, self.uvStorage ) ):
					uvValue = self.uvStorage.get()[-2:]

				#get nmlVal
				nmlValue = None
				if( self.nmlMapID is not None and self.polyAccessor.MapEvaluate( self.nmlMapID, vID, self.nmlStorage ) ):
					nmlValue = self.nmlStorage.get()

				self.vertAccessor.Select( vID )
				for j in range( self.vertAccessor.PolygonCount() ):
					neigh_pID = self.vertAccessor.PolygonByIndex( j )
					if( neigh_pID not in outerSet and neigh_pID not in innerSet ):
						self.polyAccessor.Select( neigh_pID )

						#get material name and smoothing group to check against
						stringTag = lx.object.StringTag( self.polyAccessor )
						matPTagCheckAgainst = ''
						try: matPTagCheckAgainst = stringTag.Get( lx.symbol.i_POLYTAG_MATERIAL )
						except LookupError: pass
						smthPTagCheckAgainst = ''
						try: smthPTagCheckAgainst = stringTag.Get( lx.symbol.i_POLYTAG_SMOOTHING_GROUP )
						except LookupError: pass

						#get uvVal to check against
						uvValueCheckAgainst = None
						if( self.uvMapID is not None and self.polyAccessor.MapEvaluate( self.uvMapID, vID, self.uvStorage ) ):
							 uvValueCheckAgainst = self.uvStorage.get()

						#get nmlVal to check against
						nmlValueCheckAgainst = None
						if( self.nmlMapID is not None and self.polyAccessor.MapEvaluate( self.nmlMapID, vID, self.nmlStorage ) ):
							nmlValueCheckAgainst = self.nmlStorage.get()

						#check if smoothing is continuous across ptags
						ptagContinuous = False
						if ( matPTag == matPTagCheckAgainst and smthPTag == smthPTagCheckAgainst ):
							ptagContinuous = True

						#if smoothing is continuous across ptags, check to see if the angle between the two polys is larger than the smoothing angle for that material ptag
						if ( ptagContinuous ):
							polyNormalCheckAgainst = self.polyAccessor.Normal()
							dot = ( polyNormal[0] * polyNormalCheckAgainst[0] ) + ( polyNormal[1] * polyNormalCheckAgainst[1] ) + ( polyNormal[2] * polyNormalCheckAgainst[2] )
							angle = math.acos( dot )							
							if ( math.degrees( angle ) > self.smAngleDict[matPTag] ):
								ptagContinuous = False

						#check of uv continuity
						uvContinuous = True
						if ( uvValue is None or uvValueCheckAgainst is None ):
							if ( uvValue is not None or uvValueCheckAgainst is not None ):
								uvContinuous = False
						else:
							for k in range( len( uvValue ) ):
								if ( not almost_equal( uvValue[k], uvValueCheckAgainst[k] ) ):
									uvContinuous = False
									break

						#check for vnorm continuity
						nmlContinuous = True
						if ( nmlValue is None or nmlValueCheckAgainst is None ):
							if ( nmlValue is not None or nmlValueCheckAgainst is not None ):
								nmlContinuous = False
						else:
							for k in range( len( nmlValue ) ):
								if ( not almost_equal( nmlValue[k], nmlValueCheckAgainst[k] ) ):
									nmlContinuous = False
									break

						if ( ptagContinuous ):
							print math.degrees( angle ), self.smAngleDict[matPTag]
							print uvContinuous, nmlContinuous
							print ''

						if( uvContinuous and nmlContinuous and ptagContinuous ):
							outerSet.append( neigh_pID )
							break

				self.polyAccessor.Select( pID )

		if( len( innerSet ) > 0 ):
			self.surfaceNameList.append( matPTag )
			self.surfaceList.append( innerSet )

class _markComponents( lxifc.Visitor ):
	'''Visitor object used for marking verts/edges/polys'''
	def __init__( self, componentAccessor, mark ):
		self.componentAccessor = componentAccessor
		self.mark = mark

	def vis_Evaluate( self ):
		self.componentAccessor.SetMarks( self.mark )

class _queryMapsVisitor( lxifc.Visitor ):
	def __init__( self, meshMap, stype=None ):
		self.meshMap = meshMap #MeshMapAccessor
		self.mapIDs = []
		self.stype = stype #vert map lx symbol

	def vis_Evaluate( self ):
		if( self.stype is None ):
			self.mapIDs.append( self.meshMap.ID() )
		else:
			if( self.meshMap.Type() == self.stype ):
				self.mapIDs.append( self.meshMap.ID() )

def save( absolutePath ):
	scn_svc = lxu.service.Scene()

	surfaceCount = 0
	fileData = ''
	for item in lxu.select.ItemSelection().current():
		if( scn_svc.ItemTypeName( item.Type() ) == lx.symbol.sITYPE_MESH ):
			geoObj = Geometry( item )
			result = geoObj.initStrData()
			if ( result == 0 ):
				surfaceCount += geoObj.surfaceCount
				fileData += geoObj.stringData
			elif ( result == 1 ):
				errorDialog( 'ERROR :: No UV map detected for mesh {}'.format( item.UniqueName() ) )
			elif ( result == 2 ):
				errorDialog( 'ERROR :: No VNormal map detected for mesh {}'.format( item.UniqueName() ) )

	scene = lxu.select.SceneSelection().current()
	headerStr = 'h {{{}}} {}\n\n'.format( scene.Filename().replace( ' ', '%32' ), surfaceCount )
	fs = open( absolutePath, 'w+' )
	fs.write( headerStr )
	fs.write( fileData )
	fs.close()

class Geometry():
	def __init__( self, item ):
		self._item = item
		scene = item.Context()
		time = lx.service.Selection().GetTime()
		chanRead = scene.Channels( lx.symbol.s_ACTIONLAYER_ANIM, time )
		read_mesh_obj = chanRead.ValueObj( item, item.ChannelLookup( lx.symbol.sICHAN_MESH_MESH ) )

		self._mesh = lx.object.Mesh( read_mesh_obj )
		self._vertAccessor = lx.object.Point( self._mesh.PointAccessor() )
		self._polyAccessor = lx.object.Polygon( self._mesh.PolygonAccessor() )

		self._mesh_svc = lx.service.Mesh()
		self._mark1 = self._mesh_svc.ModeCompose( lx.symbol.sMARK_USER_5, None )
		self._clearMark1 = self._mesh_svc.ModeCompose( None, lx.symbol.sMARK_USER_5 )
		self._mark2 = self._mesh_svc.ModeCompose( lx.symbol.sMARK_USER_4, None )
		self._clearMark2 = self._mesh_svc.ModeCompose( None, lx.symbol.sMARK_USER_4 )

		self._uvMapID = -1
		self._nmlMapID = -1
		self._storage = lx.object.storage( 'f', 3 )
		self._uvStorage = lx.object.storage( 'f', 2 )
		self._nmlStorage = lx.object.storage( 'f', 3 )

		self.surfaceCount = 0
		self.stringData = ''

	def getMapIDs( self, stype=lx.symbol.i_VMAP_TEXTUREUV ):
		mapAccessor = lx.object.MeshMap( self._mesh.MeshMapAccessor() )
		meshMapVisitor = _queryMapsVisitor( mapAccessor, stype )
		mapAccessor.Enumerate( lx.symbol.iMARK_ANY, meshMapVisitor, 0 )
		mapIDList = meshMapVisitor.mapIDs
		return mapIDList
		
	def initStrData( self ):
		#create marks
		clearAllMark = self._mesh_svc.ModeCompose( None, '{} {}'.format( lx.symbol.sMARK_USER_4, lx.symbol.sMARK_USER_5 ) )
		setAllMark = self._mesh_svc.ModeCompose( '{} {}'.format( lx.symbol.sMARK_USER_4, lx.symbol.sMARK_USER_5 ), None )

		#clear marks that may interfere with function
		markVisitor = _markComponents( self._polyAccessor, setAllMark )
		self._polyAccessor.Enumerate( lx.symbol.iMARK_ANY, markVisitor, 0 )
		markVisitor = _markComponents( self._vertAccessor, setAllMark )
		self._vertAccessor.Enumerate( lx.symbol.iMARK_ANY, markVisitor, 0 )

		#get uvmap
		uvMapIDS = self.getMapIDs( stype=lx.symbol.i_VMAP_TEXTUREUV )
		if ( len( uvMapIDS ) < 1 ):
			self._uvMapID = None
		else:
			self._uvMapID = uvMapIDS[0]

		#get vnmlmap
		nmlMapIDs = self.getMapIDs( stype=lx.symbol.i_VMAP_NORMAL )
		if ( len( nmlMapIDs ) < 1 ):
			self._nmlMapID = None
		else:
			self._nmlMapID = nmlMapIDs[0]

		visitorInstance = _surfaceVisitor( self._mesh, self._polyAccessor, self._item, self._uvMapID, self._nmlMapID, self._clearMark2 )
		self._polyAccessor.Enumerate( self._mark2, visitorInstance, 0 )

		#clear marks
		markVisitor = _markComponents( self._polyAccessor, clearAllMark )
		self._polyAccessor.Enumerate( lx.symbol.iMARK_ANY, markVisitor, 0 )
		markVisitor = _markComponents( self._vertAccessor, clearAllMark )
		self._vertAccessor.Enumerate( lx.symbol.iMARK_ANY, markVisitor, 0 )

		#populate geo data
		surfaceCount = len( visitorInstance.surfaceList )
		self.surfaceCount += surfaceCount
		fileStr = ''
		for i in range( surfaceCount ):
			surfStr = self.surfaceString( visitorInstance.surfaceNameList[i], visitorInstance.surfaceList[i] )
			fileStr += surfStr
		self.stringData = fileStr

		return True

	def surfaceString( self, name, surface ):
		tempPolyAccessor = lx.object.Polygon( self._mesh.PolygonAccessor() )

		#create empty list with initial sizes
		vIdxLookup = [None] * self._mesh.PointCount() #list associates vert index with the index of that vert in the surface
		verts = [] #stores list of vert class objects
		polys = [None] * len( surface ) #list of index lists. Each index points to a vert in the verts list
		for i in range( len( polys ) ):
			polys[i] = []

		#file verts and polys for surface
		vCounter = 0
		for n, pID in enumerate( surface ):
			self._polyAccessor.Select( pID )

			vCount = self._polyAccessor.VertexCount()
			polyVerts = [None] * vCount

			for i in range( vCount ):
				vID = self._polyAccessor.VertexByIndex( i )
				self._vertAccessor.Select( vID )

				#if vert is marked, then the Vert class object already exists and its index in the surface must be fetched from vIdxLookup
				if ( self._vertAccessor.TestMarks( self._mark1 ) ):
					polyVerts[i] = vIdxLookup[self._vertAccessor.Index()]

				else:
					vPos = self._vertAccessor.Pos()

					uvValue = ( 0.0, 0.0 )
					if( self._uvStorage is not None and self._polyAccessor.MapEvaluate( self._uvMapID, vID, self._uvStorage ) ):
						uvValue = self._uvStorage.get()

					nmlValue = ( 0.0, 0.0, 0.0 )
					if( self._nmlMapID is not None and self._polyAccessor.MapEvaluate( self._nmlMapID, vID, self._nmlStorage ) ):
						nmlValue = self._nmlStorage.get()
					else:
						accumulatedNormal = [ 0.0, 0.0, 0.0 ]
						for j in range( self._vertAccessor.PolygonCount() ):
							testPID = self._vertAccessor.PolygonByIndex( j )
							#figure out if testPID is smoothed with pID

							if ( testPID in surface ):
								tempPolyAccessor.Select( testPID )
								testNml = tempPolyAccessor.Normal()
								accumulatedNormal[0] += testNml[0]
								accumulatedNormal[1] += testNml[1]
								accumulatedNormal[2] += testNml[2]
						accumulatedNormalLength = math.sqrt( 	( accumulatedNormal[0] * accumulatedNormal[0] ) +
																( accumulatedNormal[1] * accumulatedNormal[1] ) +
																( accumulatedNormal[2] * accumulatedNormal[2] ) )
						accumulatedNormal[0] /= accumulatedNormalLength
						accumulatedNormal[1] /= accumulatedNormalLength
						accumulatedNormal[2] /= accumulatedNormalLength
						nmlValue = tuple( accumulatedNormal )

					verts.append( Vert( vPos, nmlValue, uvValue ) )
					vIdxLookup[self._vertAccessor.Index()] = vCounter
					polyVerts[i] = vCounter
					vCounter += 1
					self._vertAccessor.SetMarks( self._mark1 )

			polys[n] = polyVerts

		#convert verts and poly data into a string for file output
		returnStr = 's {} {} {} {{\n'.format( name.replace( ' ', '%32' ), vCounter, len( surface ) )
		for vert in verts:
			returnStr += '\tv {}\n'.format( vert )
		for poly in polys:
			polyStr = ''
			if ( len( poly ) < 3 ):
				continue
			for vIdx in poly:
				polyStr += '{} '.format( vIdx )
			returnStr += '\tp {}\n'.format( polyStr[:-1] )
		returnStr += '}\n'

		markVisitor = _markComponents( self._vertAccessor, self._clearMark1 )
		self._vertAccessor.Enumerate( self._mark1, markVisitor, 0 )

		return returnStr


class TestAllowAll:
	def Test( self, item ):
		return True


class MSHSaver( lxifc.Saver ):
	def sav_Save( self, source, filename, monitor ):
		#Test if we're saving using a subset object or pure scene
		subset = lx.object.SceneSubset()
		if( subset.set( source ) ):
			#Get the scene and collection from the subset
			scene = subset.GetScene()
			collection = subset.GetCollection()
		else:
			#Get the scene directly and a collection stand-in
			scene = lx.object.Scene( source )
			collection = TestAllowAll()
		
		#gather all export mesh items
		exportItems = []
		graph = lx.object.ItemGraph( scene.GraphLookup( lx.symbol.sGRAPH_SOURCE ) )
		scn_svc = lxu.service.Scene()
		for i in range( scn_svc.ItemTypeCount() ):
			typeID = scn_svc.ItemTypeByIndex( i )
			stype = scn_svc.ItemTypeName( typeID );
			if( stype != lx.symbol.sITYPE_MESH ):
				continue
				
			for j in range( scene.ItemCount( typeID ) ):
				item = scene.ItemByIndex( typeID, j )
				if( collection.Test( item ) ):
					exportItems.append( item )

		#setup monitor object
		monitor = lx.object.Monitor( monitor )
		monitor.Initialize( len( exportItems ) )

		surfaceCount = 0
		fileData = ''
		for item in exportItems:
			geoObj = Geometry( item )
			result = geoObj.initStrData()
			if ( result ):
				surfaceCount += geoObj.surfaceCount
				fileData += geoObj.stringData
			else:
				return lx.symbol.e_IO_ERROR

			#increment monitor
			try:
				monitor.Increment( 1 )
			except:
				return lx.symbol.e_ABORT

		sourceFilename = lxu.select.SceneSelection().current().Filename()
		if ( sourceFilename is not None ):
			sourceFilename = sourceFilename.replace( ' ', '%32' )
		headerStr = 'h {} {}\n\n'.format( sourceFilename, surfaceCount )
		fs = open( filename, 'w+t' )
		fs.write( headerStr )
		fs.write( fileData )
		fs.close()

		return lx.symbol.e_OK

def loadMeshFile( scene, fs ):
	#create vars for storing and tracking file
	surfaceReading = False
	surfaceCount = 0
	surfaces = []
	currentSurfaceIdx = 0
	currentMaterial = 'Default'
	currentVertIdx = 0
	currentPolyIdx = 0
	currentVertList = []
	currentPolyList = []

	#interpret file and load surfaces
	for line in fs:
		line = line.rstrip().lstrip()

		if ( surfaceReading ):
			if ( len( line ) < 1 ):
				continue
			if ( line[0] == 's' ):
				surfaceStrData = line.split( ' ' )
				currentMaterial = surfaceStrData[1]
				currentVertIdx = 0
				currentPolyIdx = 0
				currentVertList = [None] * int( surfaceStrData[2] )
				currentPolyList = [None] * int( surfaceStrData[3] )

			elif ( line[0] == 'v' ):
				vertStrData = line.split( ' ' )
				vPos = ( float( vertStrData[1] ), float( vertStrData[2] ), float( vertStrData[3] ) )
				vNorm = ( float( vertStrData[4] ), float( vertStrData[5] ), float( vertStrData[6] ) )
				vUV = ( float( vertStrData[7] ), float( vertStrData[8] ) )
				currentVertList[currentVertIdx] = Vert( vPos, vNorm, vUV )
				currentVertIdx += 1

			elif ( line[0] == 'p' ):
				polyIdxData = line.split( ' ' )[1:]
				for i in range( len( polyIdxData ) ):
					polyIdxData[i] = int( polyIdxData[i] )
				currentPolyList[currentPolyIdx] = polyIdxData
				currentPolyIdx += 1

			elif ( line[0] == '}' ):
				surfaces[currentSurfaceIdx] = Surface( currentMaterial, currentVertList, currentPolyList )
				currentSurfaceIdx += 1
				
		else:
			if ( line[0] == 'h' ):
				headerStr = line
				openIdx = line.find( '{' )
				closeIdx = line.find( '}' )
				front = line[:openIdx-1]
				back = line[closeIdx+1:]
				headerStr = front + back
				headerStrData = headerStr.split( ' ' )
				sourceLXO = line[openIdx+1:closeIdx]
				surfaceCount = int( headerStrData[1] )
				surfaces = [None] * surfaceCount
				surfaceReading = True

	return surfaces

def createGeo( scene, surfaces, useLayerScan=True ):
	#create mesh item
	meshItem = scene.ItemAdd( lx.service.Scene().ItemTypeLookup( lx.symbol.sITYPE_MESH ) )

	if ( useLayerScan ):
		layer_svc = lx.service.Layer()
		layer_scan = layer_svc.ScanAllocateItem( meshItem, lx.symbol.f_LAYERSCAN_EDIT )
		mesh = layer_scan.MeshEdit( 0 )
	else:
		chan_write = lx.object.ChannelWrite( scene.Channels( lx.symbol.s_ACTIONLAYER_SETUP, 0.0 ) )
		mesh_loc = chan_write.ValueObj( meshItem, meshItem.ChannelLookup( lx.symbol.sICHAN_MESH_MESH ) )
		mesh = lx.object.Mesh( mesh_loc )

	vertAccessor = lx.object.Point( mesh.PointAccessor() )
	polyAccessor = lx.object.Polygon( mesh.PolygonAccessor() )

	#create geo
	vertStorage = lx.object.storage()
	vertStorage.setType( 'p' )
	for i, surface in enumerate( surfaces ):
		#create new verts
		for j, vert in enumerate( surface.verts ):
			vID = vertAccessor.New( vert._pos )
			surfaces[i].vIDList[j] = vID

		#create new polys
		for poly in surface.polys:
			polyVCount = len( poly )
			polyVIDs = [None] * polyVCount
			for j in range( polyVCount ):
				polyVIDs[j] = surfaces[i].vIDList[poly[j]]

			vertStorage.setSize( len( polyVIDs ) )
			vertStorage.set( polyVIDs )
			pID = polyAccessor.New( lx.symbol.iPTYP_FACE, vertStorage, polyVCount, int( False ) )
			polyAccessor.Select( pID )
			stringTag = lx.object.StringTag( polyAccessor )
			stringTag.Set( lx.symbol.i_POLYTAG_MATERIAL, surface.name )

	if ( useLayerScan ):
		layer_scan.SetMeshChange( 0, lx.symbol.f_MESHEDIT_GEOMETRY )
		layer_scan.Apply()
		del layer_scan
	else:
		mesh.SetMeshEdits( lx.symbol.f_MESHEDIT_GEOMETRY )


	if ( useLayerScan ):
		layer_svc = lx.service.Layer()
		layer_scan = layer_svc.ScanAllocateItem( meshItem, lx.symbol.f_LAYERSCAN_EDIT )
		mesh = layer_scan.MeshEdit( 0 )

	vertAccessor = lx.object.Point( mesh.PointAccessor() )
	polyAccessor = lx.object.Polygon( mesh.PolygonAccessor() )
	mapAccessor = lx.object.MeshMap( mesh.MeshMapAccessor() )

	#create vmaps
	uvMapID = mapAccessor.New( lx.symbol.i_VMAP_TEXTUREUV, lx.eval( 'pref.value application.defaultTexture ?' ) )
	nmlMapID = mapAccessor.New( lx.symbol.i_VMAP_NORMAL, lx.eval( 'pref.value application.defaultVertexNormals ?' ) )

	#set vmap data
	uvStorage = lx.object.storage( 'f', 2 )
	nmlStorage = lx.object.storage( 'f', 3 )
	for i, surface in enumerate( surfaces ):
		for j, vert in enumerate( surface.verts ):
			vID = surface.vIDList[j]
			uvStorage.set( ( vert._uv[0], vert._uv[1] ) )
			nmlStorage.set( ( vert._norm[0], vert._norm[1], vert._norm[2] ) )
			vertAccessor.Select( vID )
			for k in range( vertAccessor.PolygonCount() ):
				pID = vertAccessor.PolygonByIndex( k )
				polyAccessor.Select( pID )
				polyAccessor.SetMapValue( vID, uvMapID, uvStorage )
				polyAccessor.SetMapValue( vID, nmlMapID, nmlStorage )

	#apply geo changes to mesh item
	if ( useLayerScan ):
		layer_scan.SetMeshChange( 0, lx.symbol.f_MESHEDIT_GEOMETRY )
		layer_scan.Apply()
		del layer_scan
	else:
		mesh.SetMeshEdits( lx.symbol.f_MESHEDIT_GEOMETRY )

class MSHLoader( lxifc.Loader ):
	def __init__( self ):
		self.scn_svc = lx.service.Scene()
		self._fh = None
		self._ld_target = None

	def load_Recognize( self, filename, loadInfo ):
		if( os.path.splitext( filename )[1].lower() != '.msh' ):
			lx.throw( lx.symbol.e_FALSE )
		self._fh = open( filename, 'rt' )

		info = lx.object.LoaderInfo( loadInfo )
		self._ld_target = lx.object.SceneLoaderTarget()
		self._ld_target.set( loadInfo )
		self._ld_target.SetRootType( lx.symbol.sITYPE_MESH )

		return lx.symbol.e_OK

	def load_Cleanup( self ):
		if( self._fh ):
			self._fh.close()
		return lx.symbol.e_OK

	def load_LoadObject( self, loadInfo, monitor, dest ):
		monitor = lx.object.Monitor( monitor )
		scene = lx.object.Scene( dest )
		surfaces = loadMeshFile( scene, self._fh )
		createGeo( scene, surfaces, useLayerScan=False )

		return lx.symbol.e_OK


class ImportMSH_Command( lxu.command.BasicCommand ):
	def __init__( self ):
		lxu.command.BasicCommand.__init__( self )
		self.dyna_Add( 'absolutePath', lx.symbol.sTYPE_STRING )

	def cmd_Flags( self ):
		return lx.symbol.fCMD_MODEL | lx.symbol.fCMD_UNDO

	def cmd_Interact( self ):
		pass

	def cmd_UserName( self ):
		return 'Import .MSH'

	def cmd_ButtonName( self ):
		return self.cmd_UserName()

	def cmd_Desc( self ):
		return 'Load data from .msh file.'

	def cmd_Tooltip( self ):
		return self.cmd_Desc()

	def cmd_ArgUserName( self, index ):
		if( index == 0 ):
			return 'absolutePath'

	def cmd_ArgDesc( self, index ):
		if( index == 0 ):
			return 'Absolut filepath of .msh file to import'

	def cmd_ArgType( self, index ):
		if( index == 0 ):
			return lx.symbol.sTYPE_STRING

	def basic_Enable( self, msg ):
		return True

	def basic_Execute( self, msg, flags ):
		if ( os.path.isfile( absolutePath ) ):
			scene = lxu.select.SceneSelection().current()
			fs = open( absolutePath, 'r' )
			surfaces = loadMeshFile( scene, fs )
			createGeo( scene, surfaces, useLayerScan=True )


lx.bless( ImportMSH_Command, 'timmy.importMSH')


saver_tags = {
	lx.symbol.sSAV_OUTCLASS: lx.symbol.a_SCENE,
	lx.symbol.sSAV_DOSTYPE : 'MSH',
	lx.symbol.sSRV_USERNAME: 'Timmy MSH',	
	lx.symbol.sSAV_SCENE_SUBSET : lx.symbol.sSUBSET_ALL
}
lx.bless( MSHSaver, SERVER_NAME, saver_tags )


loader_tags = {
	lx.symbol.sLOD_CLASSLIST: lx.symbol.a_SCENE,
	lx.symbol.sLOD_DOSPATTERN: '*.msh',
	lx.symbol.sLOD_MACPATTERN: '*.msh',
	lx.symbol.sSRV_USERNAME: 'Timmy MSH'
}
lx.bless( MSHLoader, SERVER_NAME, loader_tags )