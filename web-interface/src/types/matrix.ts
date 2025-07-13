import { mat4, vec3, vec4 } from 'gl-matrix';

export interface MatrixTransform {
  id: string;
  name: string;
  matrix: mat4;
  position: vec3;
  rotation: vec3;
  scale: vec3;
  isActive: boolean;
  blendMode: BlendMode;
  opacity: number;
}

export interface SceneElement {
  id: string;
  name: string;
  type: ElementType;
  transform: MatrixTransform;
  content: ElementContent;
  outputMappings: OutputMapping[];
  dependencies: string[];
  metadata: Record<string, any>;
}

export interface Scene {
  id: string;
  name: string;
  elements: SceneElement[];
  activeTransforms: MatrixTransform[];
  outputConfiguration: OutputConfiguration;
  timestamp: number;
  isActive: boolean;
}

export interface OutputMapping {
  id: string;
  sceneElementId: string;
  outputId: string;
  transform: MatrixTransform;
  priority: number;
  constraints: MappingConstraints;
}

export interface OutputConfiguration {
  id: string;
  name: string;
  resolution: Resolution;
  frameRate: number;
  bitrate: number;
  format: VideoFormat;
  colorSpace: ColorSpace;
  dynamicRange: DynamicRange;
}

export interface MappingConstraints {
  minScale: number;
  maxScale: number;
  preserveAspectRatio: boolean;
  allowRotation: boolean;
  clampToBounds: boolean;
  interpolationMode: InterpolationMode;
}

export interface Resolution {
  width: number;
  height: number;
  pixelAspectRatio: number;
}

export interface ElementContent {
  type: ContentType;
  source: string;
  properties: Record<string, any>;
  filters: Filter[];
}

export interface Filter {
  id: string;
  name: string;
  type: FilterType;
  parameters: Record<string, any>;
  isEnabled: boolean;
}

export enum ElementType {
  VIDEO = 'video',
  AUDIO = 'audio',
  IMAGE = 'image',
  TEXT = 'text',
  GEOMETRY = 'geometry',
  PARTICLE = 'particle',
  OVERLAY = 'overlay',
  EFFECT = 'effect'
}

export enum ContentType {
  STREAM = 'stream',
  FILE = 'file',
  GENERATED = 'generated',
  REALTIME = 'realtime'
}

export enum BlendMode {
  NORMAL = 'normal',
  MULTIPLY = 'multiply',
  SCREEN = 'screen',
  OVERLAY = 'overlay',
  SOFT_LIGHT = 'soft_light',
  HARD_LIGHT = 'hard_light',
  COLOR_DODGE = 'color_dodge',
  COLOR_BURN = 'color_burn',
  DARKEN = 'darken',
  LIGHTEN = 'lighten',
  DIFFERENCE = 'difference',
  EXCLUSION = 'exclusion'
}

export enum VideoFormat {
  MPEG2 = 'mpeg2',
  H264 = 'h264',
  H265 = 'h265',
  AV1 = 'av1',
  VP9 = 'vp9',
  PRORES = 'prores'
}

export enum ColorSpace {
  REC709 = 'rec709',
  REC2020 = 'rec2020',
  SRGB = 'srgb',
  DCI_P3 = 'dci_p3',
  ADOBE_RGB = 'adobe_rgb'
}

export enum DynamicRange {
  SDR = 'sdr',
  HDR10 = 'hdr10',
  HDR10_PLUS = 'hdr10_plus',
  DOLBY_VISION = 'dolby_vision',
  HLG = 'hlg'
}

export enum InterpolationMode {
  NEAREST = 'nearest',
  LINEAR = 'linear',
  CUBIC = 'cubic',
  LANCZOS = 'lanczos'
}

export enum FilterType {
  COLOR_CORRECTION = 'color_correction',
  BLUR = 'blur',
  SHARPEN = 'sharpen',
  NOISE_REDUCTION = 'noise_reduction',
  CHROMAKEY = 'chromakey',
  TRANSFORM = 'transform',
  DISTORTION = 'distortion',
  TEMPORAL = 'temporal'
}

export interface MatrixLayerState {
  layers: MatrixLayer[];
  activeLayerId: string | null;
  transformMatrix: mat4;
  viewMatrix: mat4;
  projectionMatrix: mat4;
}

export interface MatrixLayer {
  id: string;
  name: string;
  elements: SceneElement[];
  transform: MatrixTransform;
  isVisible: boolean;
  isLocked: boolean;
  blendMode: BlendMode;
  opacity: number;
}

export interface EncodingStats {
  fps: number;
  bitrate: number;
  quality: number;
  latency: number;
  droppedFrames: number;
  cpuUsage: number;
  memoryUsage: number;
  gpuUsage: number;
}