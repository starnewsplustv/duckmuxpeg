import React, { useRef, useEffect, useMemo, useState } from 'react';
import { Canvas, useFrame, useThree } from '@react-three/fiber';
import { Points, PointMaterial, OrbitControls } from '@react-three/drei';
import * as THREE from 'three';
import { motion } from 'framer-motion';
import styled, { keyframes } from 'styled-components';

interface SpacescapeBackgroundProps {
  intensity?: number;
  particleCount?: number;
  nebulaDensity?: number;
  reactToAudio?: boolean;
  reactToMouse?: boolean;
  autoRotate?: boolean;
  showNebula?: boolean;
  showStarfield?: boolean;
  showAurora?: boolean;
  className?: string;
}

// Animated background container
const BackgroundContainer = styled.div`
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  z-index: -1;
  overflow: hidden;
`;

// Aurora effect animation
const auroraAnimation = keyframes`
  0% { 
    background-position: 0% 50%;
    opacity: 0.3;
  }
  25% { 
    background-position: 100% 50%;
    opacity: 0.5;
  }
  50% { 
    background-position: 50% 100%;
    opacity: 0.4;
  }
  75% { 
    background-position: 100% 0%;
    opacity: 0.6;
  }
  100% { 
    background-position: 0% 50%;
    opacity: 0.3;
  }
`;

const AuroraLayer = styled.div<{ isActive: boolean }>`
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: linear-gradient(
    45deg,
    rgba(0, 255, 255, 0.1),
    rgba(191, 0, 255, 0.15),
    rgba(255, 0, 191, 0.1),
    rgba(0, 255, 127, 0.12),
    rgba(255, 127, 0, 0.08)
  );
  background-size: 400% 400%;
  animation: ${auroraAnimation} 12s ease-in-out infinite;
  opacity: ${props => props.isActive ? 1 : 0};
  transition: opacity 2s ease;
  mix-blend-mode: screen;
`;

// Nebula effect with CSS
const nebulaGlow = keyframes`
  0%, 100% { 
    transform: scale(1) rotate(0deg);
    opacity: 0.2;
  }
  50% { 
    transform: scale(1.1) rotate(180deg);
    opacity: 0.4;
  }
`;

const NebulaLayer = styled.div<{ isActive: boolean; density: number }>`
  position: absolute;
  top: -25%;
  left: -25%;
  width: 150%;
  height: 150%;
  background: radial-gradient(
    ellipse at 30% 70%,
    rgba(0, 212, 255, ${props => props.density * 0.1}) 0%,
    rgba(191, 0, 255, ${props => props.density * 0.08}) 25%,
    rgba(255, 0, 191, ${props => props.density * 0.06}) 50%,
    transparent 70%
  ),
  radial-gradient(
    ellipse at 70% 30%,
    rgba(255, 127, 0, ${props => props.density * 0.05}) 0%,
    rgba(0, 255, 127, ${props => props.density * 0.04}) 40%,
    transparent 70%
  );
  animation: ${nebulaGlow} 20s ease-in-out infinite;
  opacity: ${props => props.isActive ? 1 : 0};
  transition: opacity 3s ease;
  mix-blend-mode: screen;
  filter: blur(2px);
`;

// Star field component
const StarField: React.FC<{ count: number; speed: number }> = ({ count, speed }) => {
  const points = useRef<THREE.Points>(null!);
  const [positions, colors] = useMemo(() => {
    const positions = new Float32Array(count * 3);
    const colors = new Float32Array(count * 3);
    
    for (let i = 0; i < count; i++) {
      // Random positions in a sphere
      const radius = Math.random() * 50 + 10;
      const theta = Math.random() * Math.PI * 2;
      const phi = Math.acos(2 * Math.random() - 1);
      
      positions[i * 3] = radius * Math.sin(phi) * Math.cos(theta);
      positions[i * 3 + 1] = radius * Math.sin(phi) * Math.sin(theta);
      positions[i * 3 + 2] = radius * Math.cos(phi);
      
      // Random colors with space theme
      const colorChoice = Math.random();
      if (colorChoice < 0.3) {
        // Blue-white stars
        colors[i * 3] = 0.8 + Math.random() * 0.2;
        colors[i * 3 + 1] = 0.9 + Math.random() * 0.1;
        colors[i * 3 + 2] = 1.0;
      } else if (colorChoice < 0.6) {
        // Yellow-white stars
        colors[i * 3] = 1.0;
        colors[i * 3 + 1] = 0.9 + Math.random() * 0.1;
        colors[i * 3 + 2] = 0.7 + Math.random() * 0.2;
      } else {
        // Red-orange stars
        colors[i * 3] = 1.0;
        colors[i * 3 + 1] = 0.6 + Math.random() * 0.3;
        colors[i * 3 + 2] = 0.3 + Math.random() * 0.3;
      }
    }
    
    return [positions, colors];
  }, [count]);

  useFrame((state) => {
    if (points.current) {
      points.current.rotation.x += speed * 0.0001;
      points.current.rotation.y += speed * 0.0002;
      
      // Twinkle effect
      const time = state.clock.getElapsedTime();
      const geometry = points.current.geometry;
      const colorsArray = geometry.attributes.color.array as Float32Array;
      
      for (let i = 0; i < count; i++) {
        const twinkle = 0.7 + 0.3 * Math.sin(time * (1 + i * 0.01));
        colorsArray[i * 3] = colors[i * 3] * twinkle;
        colorsArray[i * 3 + 1] = colors[i * 3 + 1] * twinkle;
        colorsArray[i * 3 + 2] = colors[i * 3 + 2] * twinkle;
      }
      
      geometry.attributes.color.needsUpdate = true;
    }
  });

  return (
    <Points ref={points} positions={positions}>
      <PointMaterial
        transparent
        vertexColors
        size={2}
        sizeAttenuation={true}
        depthWrite={false}
        blending={THREE.AdditiveBlending}
      />
    </Points>
  );
};

// Particle nebula component
const ParticleNebula: React.FC<{ density: number; reactToMouse: boolean; mousePosition: { x: number; y: number } }> = ({
  density,
  reactToMouse,
  mousePosition
}) => {
  const points = useRef<THREE.Points>(null!);
  const particleCount = Math.floor(density * 500);
  
  const [positions, colors] = useMemo(() => {
    const positions = new Float32Array(particleCount * 3);
    const colors = new Float32Array(particleCount * 3);
    
    for (let i = 0; i < particleCount; i++) {
      // Clustered positions for nebula effect
      const clusterX = (Math.random() - 0.5) * 30;
      const clusterY = (Math.random() - 0.5) * 30;
      const clusterZ = (Math.random() - 0.5) * 30;
      
      positions[i * 3] = clusterX + (Math.random() - 0.5) * 10;
      positions[i * 3 + 1] = clusterY + (Math.random() - 0.5) * 10;
      positions[i * 3 + 2] = clusterZ + (Math.random() - 0.5) * 10;
      
      // Nebula colors - purples, blues, and pinks
      const colorType = Math.random();
      if (colorType < 0.4) {
        colors[i * 3] = 0.8 + Math.random() * 0.2;     // R
        colors[i * 3 + 1] = 0.2 + Math.random() * 0.4; // G
        colors[i * 3 + 2] = 1.0;                        // B (Purple-Blue)
      } else if (colorType < 0.7) {
        colors[i * 3] = 1.0;                            // R
        colors[i * 3 + 1] = 0.1 + Math.random() * 0.3; // G
        colors[i * 3 + 2] = 0.8 + Math.random() * 0.2; // B (Pink-Purple)
      } else {
        colors[i * 3] = 0.2 + Math.random() * 0.3;     // R
        colors[i * 3 + 1] = 0.8 + Math.random() * 0.2; // G
        colors[i * 3 + 2] = 1.0;                        // B (Cyan-Blue)
      }
    }
    
    return [positions, colors];
  }, [particleCount]);

  useFrame((state) => {
    if (points.current) {
      const time = state.clock.getElapsedTime();
      
      // Gentle rotation
      points.current.rotation.x += 0.0005;
      points.current.rotation.y += 0.0003;
      
      // Mouse interaction
      if (reactToMouse) {
        const targetX = (mousePosition.x - 0.5) * 2;
        const targetY = -(mousePosition.y - 0.5) * 2;
        
        points.current.rotation.x += targetY * 0.001;
        points.current.rotation.y += targetX * 0.001;
      }
      
      // Animate particle positions for floating effect
      const positionsArray = points.current.geometry.attributes.position.array as Float32Array;
      for (let i = 0; i < particleCount; i++) {
        const originalY = positions[i * 3 + 1];
        positionsArray[i * 3 + 1] = originalY + Math.sin(time * 0.5 + i * 0.01) * 0.5;
      }
      points.current.geometry.attributes.position.needsUpdate = true;
    }
  });

  return (
    <Points ref={points} positions={positions}>
      <PointMaterial
        transparent
        vertexColors
        size={1.5}
        sizeAttenuation={true}
        depthWrite={false}
        blending={THREE.AdditiveBlending}
        opacity={0.6}
      />
    </Points>
  );
};

// Main 3D scene component
const SpaceScene: React.FC<{
  particleCount: number;
  nebulaDensity: number;
  autoRotate: boolean;
  reactToMouse: boolean;
  mousePosition: { x: number; y: number };
}> = ({ particleCount, nebulaDensity, autoRotate, reactToMouse, mousePosition }) => {
  const { camera } = useThree();

  useEffect(() => {
    camera.position.set(0, 0, 30);
  }, [camera]);

  return (
    <>
      <StarField count={particleCount} speed={autoRotate ? 1 : 0.3} />
      <ParticleNebula
        density={nebulaDensity}
        reactToMouse={reactToMouse}
        mousePosition={mousePosition}
      />
      
      {/* Ambient lighting for subtle illumination */}
      <ambientLight intensity={0.1} color="#ffffff" />
      <pointLight position={[10, 10, 10]} intensity={0.3} color="#00d4ff" />
      <pointLight position={[-10, -10, -10]} intensity={0.2} color="#bf00ff" />
    </>
  );
};

export const SpacescapeBackground: React.FC<SpacescapeBackgroundProps> = ({
  intensity = 1,
  particleCount = 2000,
  nebulaDensity = 0.8,
  reactToAudio = false,
  reactToMouse = true,
  autoRotate = true,
  showNebula = true,
  showStarfield = true,
  showAurora = true,
  className
}) => {
  const [mousePosition, setMousePosition] = useState({ x: 0.5, y: 0.5 });
  const [audioLevel, setAudioLevel] = useState(0);

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (reactToMouse) {
        setMousePosition({
          x: e.clientX / window.innerWidth,
          y: e.clientY / window.innerHeight
        });
      }
    };

    window.addEventListener('mousemove', handleMouseMove);
    return () => window.removeEventListener('mousemove', handleMouseMove);
  }, [reactToMouse]);

  // Audio reactivity setup (placeholder for future implementation)
  useEffect(() => {
    if (reactToAudio) {
      // Here you would set up Web Audio API to analyze audio
      // and update audioLevel state based on frequency analysis
      // This is a simplified placeholder
      const interval = setInterval(() => {
        setAudioLevel(Math.random() * 0.5 + 0.5);
      }, 100);
      
      return () => clearInterval(interval);
    }
  }, [reactToAudio]);

  const effectiveIntensity = reactToAudio ? intensity * (0.5 + audioLevel * 0.5) : intensity;

  return (
    <BackgroundContainer className={className}>
      {/* CSS-based effects */}
      {showAurora && (
        <AuroraLayer isActive={true} />
      )}
      
      {showNebula && (
        <NebulaLayer 
          isActive={true} 
          density={nebulaDensity * effectiveIntensity}
        />
      )}
      
      {/* 3D Canvas for particles */}
      {showStarfield && (
        <Canvas
          camera={{ fov: 75, position: [0, 0, 30] }}
          style={{ background: 'transparent' }}
        >
          <SpaceScene
            particleCount={Math.floor(particleCount * effectiveIntensity)}
            nebulaDensity={nebulaDensity * effectiveIntensity}
            autoRotate={autoRotate}
            reactToMouse={reactToMouse}
            mousePosition={mousePosition}
          />
        </Canvas>
      )}
      
      {/* Gradient overlay for depth */}
      <div
        style={{
          position: 'absolute',
          top: 0,
          left: 0,
          width: '100%',
          height: '100%',
          background: 'radial-gradient(ellipse at center, transparent 0%, rgba(10, 10, 15, 0.3) 70%, rgba(10, 10, 15, 0.7) 100%)',
          pointerEvents: 'none'
        }}
      />
    </BackgroundContainer>
  );
};