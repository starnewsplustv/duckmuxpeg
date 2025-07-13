import { create } from 'zustand';
import { subscribeWithSelector } from 'zustand/middleware';
import { mat4, vec3 } from 'gl-matrix';
import {
  Scene,
  SceneElement,
  MatrixTransform,
  OutputMapping,
  MatrixLayerState,
  MatrixLayer,
  EncodingStats,
  ElementType,
  BlendMode
} from '../types/matrix';

interface MatrixStore {
  // Scene Management
  scenes: Scene[];
  activeSceneId: string | null;
  
  // Matrix Layers
  matrixState: MatrixLayerState;
  
  // Output Management
  outputMappings: OutputMapping[];
  
  // Performance & Stats
  stats: EncodingStats;
  isRecording: boolean;
  
  // Actions
  addScene: (scene: Omit<Scene, 'id' | 'timestamp'>) => string;
  removeScene: (sceneId: string) => void;
  setActiveScene: (sceneId: string) => void;
  
  // Element Management
  addElement: (sceneId: string, element: Omit<SceneElement, 'id'>) => string;
  removeElement: (sceneId: string, elementId: string) => void;
  updateElement: (sceneId: string, elementId: string, updates: Partial<SceneElement>) => void;
  
  // Matrix Operations
  updateElementTransform: (sceneId: string, elementId: string, transform: Partial<MatrixTransform>) => void;
  applyMatrixTransform: (elementId: string, matrix: mat4) => void;
  resetTransforms: (sceneId: string) => void;
  
  // Layer Management
  addLayer: (layer: Omit<MatrixLayer, 'id'>) => string;
  removeLayer: (layerId: string) => void;
  updateLayer: (layerId: string, updates: Partial<MatrixLayer>) => void;
  setActiveLayer: (layerId: string) => void;
  
  // Output Mapping
  addOutputMapping: (mapping: Omit<OutputMapping, 'id'>) => string;
  removeOutputMapping: (mappingId: string) => void;
  updateOutputMapping: (mappingId: string, updates: Partial<OutputMapping>) => void;
  
  // Matrix Calculations
  calculateCompositeMatrix: (elementId: string) => mat4;
  getElementScreenPosition: (elementId: string) => vec3;
  
  // Performance
  updateStats: (stats: Partial<EncodingStats>) => void;
  startRecording: () => void;
  stopRecording: () => void;
}

const createDefaultTransform = (): MatrixTransform => ({
  id: crypto.randomUUID(),
  name: 'Default Transform',
  matrix: mat4.create(),
  position: vec3.fromValues(0, 0, 0),
  rotation: vec3.fromValues(0, 0, 0),
  scale: vec3.fromValues(1, 1, 1),
  isActive: true,
  blendMode: BlendMode.NORMAL,
  opacity: 1.0
});

const createDefaultMatrixState = (): MatrixLayerState => ({
  layers: [],
  activeLayerId: null,
  transformMatrix: mat4.create(),
  viewMatrix: mat4.create(),
  projectionMatrix: mat4.create()
});

export const useMatrixStore = create<MatrixStore>()(
  subscribeWithSelector((set, get) => ({
    scenes: [],
    activeSceneId: null,
    matrixState: createDefaultMatrixState(),
    outputMappings: [],
    stats: {
      fps: 0,
      bitrate: 0,
      quality: 0,
      latency: 0,
      droppedFrames: 0,
      cpuUsage: 0,
      memoryUsage: 0,
      gpuUsage: 0
    },
    isRecording: false,

    addScene: (sceneData) => {
      const id = crypto.randomUUID();
      const scene: Scene = {
        ...sceneData,
        id,
        timestamp: Date.now()
      };
      
      set((state) => ({
        scenes: [...state.scenes, scene],
        activeSceneId: state.activeSceneId || id
      }));
      
      return id;
    },

    removeScene: (sceneId) => {
      set((state) => ({
        scenes: state.scenes.filter(s => s.id !== sceneId),
        activeSceneId: state.activeSceneId === sceneId ? 
          state.scenes.find(s => s.id !== sceneId)?.id || null : 
          state.activeSceneId
      }));
    },

    setActiveScene: (sceneId) => {
      set({ activeSceneId: sceneId });
    },

    addElement: (sceneId, elementData) => {
      const id = crypto.randomUUID();
      const element: SceneElement = {
        ...elementData,
        id,
        transform: elementData.transform || createDefaultTransform()
      };

      set((state) => ({
        scenes: state.scenes.map(scene =>
          scene.id === sceneId
            ? { ...scene, elements: [...scene.elements, element] }
            : scene
        )
      }));

      return id;
    },

    removeElement: (sceneId, elementId) => {
      set((state) => ({
        scenes: state.scenes.map(scene =>
          scene.id === sceneId
            ? { ...scene, elements: scene.elements.filter(e => e.id !== elementId) }
            : scene
        ),
        outputMappings: state.outputMappings.filter(m => m.sceneElementId !== elementId)
      }));
    },

    updateElement: (sceneId, elementId, updates) => {
      set((state) => ({
        scenes: state.scenes.map(scene =>
          scene.id === sceneId
            ? {
                ...scene,
                elements: scene.elements.map(element =>
                  element.id === elementId ? { ...element, ...updates } : element
                )
              }
            : scene
        )
      }));
    },

    updateElementTransform: (sceneId, elementId, transformUpdates) => {
      set((state) => ({
        scenes: state.scenes.map(scene =>
          scene.id === sceneId
            ? {
                ...scene,
                elements: scene.elements.map(element =>
                  element.id === elementId
                    ? {
                        ...element,
                        transform: { ...element.transform, ...transformUpdates }
                      }
                    : element
                )
              }
            : scene
        )
      }));
    },

    applyMatrixTransform: (elementId, matrix) => {
      const state = get();
      const activeScene = state.scenes.find(s => s.id === state.activeSceneId);
      if (!activeScene) return;

      const element = activeScene.elements.find(e => e.id === elementId);
      if (!element) return;

      const newMatrix = mat4.create();
      mat4.multiply(newMatrix, matrix, element.transform.matrix);

      get().updateElementTransform(activeScene.id, elementId, { matrix: newMatrix });
    },

    resetTransforms: (sceneId) => {
      set((state) => ({
        scenes: state.scenes.map(scene =>
          scene.id === sceneId
            ? {
                ...scene,
                elements: scene.elements.map(element => ({
                  ...element,
                  transform: createDefaultTransform()
                }))
              }
            : scene
        )
      }));
    },

    addLayer: (layerData) => {
      const id = crypto.randomUUID();
      const layer: MatrixLayer = { ...layerData, id };

      set((state) => ({
        matrixState: {
          ...state.matrixState,
          layers: [...state.matrixState.layers, layer],
          activeLayerId: state.matrixState.activeLayerId || id
        }
      }));

      return id;
    },

    removeLayer: (layerId) => {
      set((state) => ({
        matrixState: {
          ...state.matrixState,
          layers: state.matrixState.layers.filter(l => l.id !== layerId),
          activeLayerId: state.matrixState.activeLayerId === layerId ?
            state.matrixState.layers.find(l => l.id !== layerId)?.id || null :
            state.matrixState.activeLayerId
        }
      }));
    },

    updateLayer: (layerId, updates) => {
      set((state) => ({
        matrixState: {
          ...state.matrixState,
          layers: state.matrixState.layers.map(layer =>
            layer.id === layerId ? { ...layer, ...updates } : layer
          )
        }
      }));
    },

    setActiveLayer: (layerId) => {
      set((state) => ({
        matrixState: {
          ...state.matrixState,
          activeLayerId: layerId
        }
      }));
    },

    addOutputMapping: (mappingData) => {
      const id = crypto.randomUUID();
      const mapping: OutputMapping = { ...mappingData, id };

      set((state) => ({
        outputMappings: [...state.outputMappings, mapping]
      }));

      return id;
    },

    removeOutputMapping: (mappingId) => {
      set((state) => ({
        outputMappings: state.outputMappings.filter(m => m.id !== mappingId)
      }));
    },

    updateOutputMapping: (mappingId, updates) => {
      set((state) => ({
        outputMappings: state.outputMappings.map(mapping =>
          mapping.id === mappingId ? { ...mapping, ...updates } : mapping
        )
      }));
    },

    calculateCompositeMatrix: (elementId) => {
      const state = get();
      const activeScene = state.scenes.find(s => s.id === state.activeSceneId);
      if (!activeScene) return mat4.create();

      const element = activeScene.elements.find(e => e.id === elementId);
      if (!element) return mat4.create();

      const compositeMatrix = mat4.create();
      
      // Apply transforms in order: scale -> rotate -> translate
      const translationMatrix = mat4.create();
      const rotationMatrix = mat4.create();
      const scaleMatrix = mat4.create();

      mat4.fromTranslation(translationMatrix, element.transform.position);
      mat4.fromRotation(rotationMatrix, element.transform.rotation[2], [0, 0, 1]);
      mat4.fromScaling(scaleMatrix, element.transform.scale);

      mat4.multiply(compositeMatrix, translationMatrix, rotationMatrix);
      mat4.multiply(compositeMatrix, compositeMatrix, scaleMatrix);
      mat4.multiply(compositeMatrix, compositeMatrix, element.transform.matrix);

      return compositeMatrix;
    },

    getElementScreenPosition: (elementId) => {
      const matrix = get().calculateCompositeMatrix(elementId);
      const position = vec3.create();
      mat4.getTranslation(position, matrix);
      return position;
    },

    updateStats: (statsUpdate) => {
      set((state) => ({
        stats: { ...state.stats, ...statsUpdate }
      }));
    },

    startRecording: () => {
      set({ isRecording: true });
    },

    stopRecording: () => {
      set({ isRecording: false });
    }
  }))
);

// Selectors for optimized component subscriptions
export const useActiveScene = () => useMatrixStore(state => 
  state.scenes.find(s => s.id === state.activeSceneId)
);

export const useActiveLayer = () => useMatrixStore(state =>
  state.matrixState.layers.find(l => l.id === state.matrixState.activeLayerId)
);

export const useElementById = (elementId: string) => useMatrixStore(state => {
  const activeScene = state.scenes.find(s => s.id === state.activeSceneId);
  return activeScene?.elements.find(e => e.id === elementId);
});