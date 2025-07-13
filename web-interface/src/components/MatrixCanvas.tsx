import React, { useRef, useEffect, useMemo } from 'react';
import { Canvas, useFrame, useThree } from '@react-three/fiber';
import { OrbitControls, Grid, Stats } from '@react-three/drei';
import { mat4, vec3 } from 'gl-matrix';
import * as THREE from 'three';
import { useMatrixStore, useActiveScene } from '../store/matrixStore';
import { SceneElement, MatrixTransform } from '../types/matrix';

interface MatrixCanvasProps {
  width: number;
  height: number;
  className?: string;
}

interface ElementMeshProps {
  element: SceneElement;
  transform: MatrixTransform;
  isSelected: boolean;
  onSelect: (elementId: string) => void;
}

const ElementMesh: React.FC<ElementMeshProps> = ({ element, transform, isSelected, onSelect }) => {
  const meshRef = useRef<THREE.Mesh>(null!);
  const { calculateCompositeMatrix } = useMatrixStore();

  useFrame(() => {
    if (meshRef.current) {
      const matrix = calculateCompositeMatrix(element.id);
      const threeMatrix = new THREE.Matrix4();
      
      // Convert gl-matrix to Three.js matrix
      threeMatrix.set(
        matrix[0], matrix[4], matrix[8], matrix[12],
        matrix[1], matrix[5], matrix[9], matrix[13],
        matrix[2], matrix[6], matrix[10], matrix[14],
        matrix[3], matrix[7], matrix[11], matrix[15]
      );
      
      meshRef.current.matrix.copy(threeMatrix);
      meshRef.current.matrixAutoUpdate = false;
    }
  });

  const color = useMemo(() => {
    switch (element.type) {
      case 'video': return '#ff6b6b';
      case 'image': return '#4ecdc4';
      case 'text': return '#45b7d1';
      case 'geometry': return '#96ceb4';
      case 'particle': return '#feca57';
      case 'overlay': return '#ff9ff3';
      case 'effect': return '#54a0ff';
      default: return '#ddd';
    }
  }, [element.type]);

  return (
    <mesh
      ref={meshRef}
      onClick={() => onSelect(element.id)}
      onPointerOver={(e) => {
        e.stopPropagation();
        document.body.style.cursor = 'pointer';
      }}
      onPointerOut={() => {
        document.body.style.cursor = 'default';
      }}
    >
      <boxGeometry args={[1, 1, 0.1]} />
      <meshPhongMaterial
        color={color}
        opacity={transform.opacity}
        transparent={transform.opacity < 1}
        wireframe={isSelected}
      />
      
      {/* Selection indicator */}
      {isSelected && (
        <mesh>
          <boxGeometry args={[1.1, 1.1, 0.15]} />
          <meshBasicMaterial color="#ffffff" wireframe opacity={0.5} transparent />
        </mesh>
      )}
    </mesh>
  );
};

const MatrixVisualization: React.FC<{ selectedElementId: string | null; onElementSelect: (id: string) => void }> = ({ 
  selectedElementId, 
  onElementSelect 
}) => {
  const { camera, gl } = useThree();
  const activeScene = useActiveScene();
  const { matrixState } = useMatrixStore();

  useEffect(() => {
    // Setup camera
    camera.position.set(5, 5, 5);
    camera.lookAt(0, 0, 0);
  }, [camera]);

  useEffect(() => {
    // Apply global transform matrices
    const { transformMatrix, viewMatrix, projectionMatrix } = matrixState;
    
    if (transformMatrix) {
      const threeMatrix = new THREE.Matrix4();
      threeMatrix.set(
        transformMatrix[0], transformMatrix[4], transformMatrix[8], transformMatrix[12],
        transformMatrix[1], transformMatrix[5], transformMatrix[9], transformMatrix[13],
        transformMatrix[2], transformMatrix[6], transformMatrix[10], transformMatrix[14],
        transformMatrix[3], transformMatrix[7], transformMatrix[11], transformMatrix[15]
      );
      
      gl.domElement.style.transform = `matrix3d(${threeMatrix.elements.join(',')})`;
    }
  }, [matrixState, gl]);

  if (!activeScene) return null;

  return (
    <group>
      {/* Render scene elements */}
      {activeScene.elements.map(element => (
        <ElementMesh
          key={element.id}
          element={element}
          transform={element.transform}
          isSelected={selectedElementId === element.id}
          onSelect={onElementSelect}
        />
      ))}
      
      {/* Matrix layers */}
      {matrixState.layers.map(layer => (
        <group key={layer.id} visible={layer.isVisible}>
          {layer.elements.map(element => (
            <ElementMesh
              key={element.id}
              element={element}
              transform={element.transform}
              isSelected={selectedElementId === element.id}
              onSelect={onElementSelect}
            />
          ))}
        </group>
      ))}
      
      {/* Coordinate system indicator */}
      <group>
        <arrowHelper args={[new THREE.Vector3(1, 0, 0), new THREE.Vector3(0, 0, 0), 2, 0xff0000]} />
        <arrowHelper args={[new THREE.Vector3(0, 1, 0), new THREE.Vector3(0, 0, 0), 2, 0x00ff00]} />
        <arrowHelper args={[new THREE.Vector3(0, 0, 1), new THREE.Vector3(0, 0, 0), 2, 0x0000ff]} />
      </group>
    </group>
  );
};

export const MatrixCanvas: React.FC<MatrixCanvasProps> = ({ width, height, className }) => {
  const [selectedElementId, setSelectedElementId] = React.useState<string | null>(null);

  return (
    <div className={className} style={{ width, height }}>
      <Canvas
        camera={{ fov: 75, position: [5, 5, 5] }}
        style={{ background: 'linear-gradient(135deg, #1e3c72 0%, #2a5298 100%)' }}
      >
        <ambientLight intensity={0.6} />
        <pointLight position={[10, 10, 10]} intensity={0.8} />
        <pointLight position={[-10, -10, -10]} intensity={0.4} />
        
        <MatrixVisualization
          selectedElementId={selectedElementId}
          onElementSelect={setSelectedElementId}
        />
        
        <Grid
          args={[20, 20]}
          position={[0, -2, 0]}
          cellSize={0.5}
          cellThickness={0.5}
          cellColor="#6f6f6f"
          sectionSize={2}
          sectionThickness={1}
          sectionColor="#9d9d9d"
          fadeDistance={50}
          fadeStrength={1}
          followCamera={false}
          infiniteGrid={true}
        />
        
        <OrbitControls
          enablePan={true}
          enableZoom={true}
          enableRotate={true}
          dampingFactor={0.05}
          enableDamping={true}
        />
        
        <Stats />
      </Canvas>
    </div>
  );
};