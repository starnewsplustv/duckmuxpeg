import React, { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import styled from 'styled-components';
import { SpacescapeBackground } from './SpacescapeBackground';
import { AdvancedButton } from './AdvancedButton';
import { DemoShowcase } from './DemoShowcase';

interface QuantumInterfaceProps {
  className?: string;
}

// Main container with glass morphism
const InterfaceContainer = styled.div`
  position: relative;
  width: 100vw;
  height: 100vh;
  overflow: hidden;
  color: white;
  font-family: var(--font-primary);
`;

// Header with floating controls
const QuantumHeader = styled(motion.header)`
  position: fixed;
  top: var(--space-lg);
  left: var(--space-lg);
  right: var(--space-lg);
  z-index: 100;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: var(--space-md) var(--space-xl);
  background: rgba(255, 255, 255, 0.05);
  backdrop-filter: blur(20px);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: var(--radius-xl);
  box-shadow: var(--shadow-depth);
`;

const Logo = styled.div`
  display: flex;
  align-items: center;
  gap: var(--space-md);
  
  h1 {
    font-size: 1.5rem;
    font-weight: 800;
    background: linear-gradient(135deg, var(--color-primary), var(--color-neon-purple));
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    margin: 0;
  }
`;

const LogoIcon = styled.div`
  width: 40px;
  height: 40px;
  background: conic-gradient(from 0deg, var(--color-neon-blue), var(--color-neon-green), var(--color-neon-orange), var(--color-neon-pink), var(--color-neon-blue));
  border-radius: var(--radius-lg);
  display: flex;
  align-items: center;
  justify-content: center;
  animation: spin 20s linear infinite;
  
  &::before {
    content: '◊';
    color: white;
    font-size: 1.2rem;
    font-weight: bold;
  }
  
  @keyframes spin {
    to { transform: rotate(360deg); }
  }
`;

const HeaderControls = styled.div`
  display: flex;
  gap: var(--space-md);
  align-items: center;
`;

// Central control panel
const ControlPanel = styled(motion.div)`
  position: fixed;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  z-index: 50;
  padding: var(--space-2xl);
  background: rgba(0, 0, 0, 0.2);
  backdrop-filter: blur(30px);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: var(--radius-xl);
  box-shadow: 0 20px 40px rgba(0, 0, 0, 0.3);
  min-width: 400px;
`;

const PanelTitle = styled.h2`
  text-align: center;
  margin-bottom: var(--space-xl);
  font-size: 1.8rem;
  font-weight: 700;
  background: linear-gradient(135deg, var(--color-neon-blue), var(--color-neon-pink));
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
`;

const ButtonGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: var(--space-lg);
  margin-bottom: var(--space-xl);
`;

const ButtonShowcase = styled.div`
  display: flex;
  flex-direction: column;
  gap: var(--space-md);
`;

// Side panels for matrix controls
const SidePanel = styled(motion.div)<{ side: 'left' | 'right' }>`
  position: fixed;
  top: 50%;
  ${props => props.side}: var(--space-lg);
  transform: translateY(-50%);
  z-index: 75;
  width: 300px;
  padding: var(--space-xl);
  background: rgba(0, 0, 0, 0.15);
  backdrop-filter: blur(25px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: var(--radius-xl);
  box-shadow: var(--shadow-depth);
`;

const MatrixVisualization = styled.div`
  width: 100%;
  height: 200px;
  background: linear-gradient(135deg, rgba(0, 255, 255, 0.1), rgba(191, 0, 255, 0.1));
  border: 1px solid rgba(0, 255, 255, 0.3);
  border-radius: var(--radius-md);
  position: relative;
  overflow: hidden;
  margin-bottom: var(--space-lg);
  
  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.1), transparent);
    animation: scan 3s ease-in-out infinite;
  }
  
  @keyframes scan {
    0% { left: -100%; }
    100% { left: 100%; }
  }
`;

const StatusIndicator = styled.div<{ status: 'active' | 'idle' | 'error' }>`
  display: flex;
  align-items: center;
  gap: var(--space-sm);
  margin-bottom: var(--space-md);
  
  &::before {
    content: '';
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: ${props => {
      switch (props.status) {
        case 'active': return 'var(--color-neon-green)';
        case 'error': return 'var(--color-secondary)';
        default: return 'var(--color-quaternary)';
      }
    }};
    box-shadow: 0 0 10px currentColor;
    animation: pulse 2s ease-in-out infinite;
  }
`;

// Performance metrics
const MetricsGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: var(--space-sm);
  margin-top: var(--space-md);
`;

const MetricCard = styled.div`
  padding: var(--space-md);
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.05);
  border-radius: var(--radius-md);
  text-align: center;
  
  .value {
    font-size: 1.2rem;
    font-weight: 700;
    color: var(--color-primary);
  }
  
  .label {
    font-size: 0.8rem;
    opacity: 0.7;
    margin-top: var(--space-xs);
  }
`;

// Bottom toolbar
const BottomToolbar = styled(motion.div)`
  position: fixed;
  bottom: var(--space-lg);
  left: 50%;
  transform: translateX(-50%);
  z-index: 100;
  display: flex;
  gap: var(--space-md);
  padding: var(--space-md) var(--space-xl);
  background: rgba(0, 0, 0, 0.3);
  backdrop-filter: blur(20px);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: var(--radius-full);
  box-shadow: var(--shadow-depth);
`;

export const QuantumInterface: React.FC<QuantumInterfaceProps> = ({ className }) => {
  const [isRecording, setIsRecording] = useState(false);
  const [selectedTool, setSelectedTool] = useState('transform');
  const [backgroundIntensity, setBackgroundIntensity] = useState(1);
  const [showMetrics, setShowMetrics] = useState(true);
  const [systemStatus, setSystemStatus] = useState<'active' | 'idle' | 'error'>('idle');

  // Demo metrics simulation
  const [metrics, setMetrics] = useState({
    fps: 60,
    cpu: 45,
    memory: 62,
    gpu: 73
  });

  useEffect(() => {
    const interval = setInterval(() => {
      setMetrics({
        fps: 58 + Math.random() * 4,
        cpu: 40 + Math.random() * 20,
        memory: 60 + Math.random() * 10,
        gpu: 70 + Math.random() * 15
      });
    }, 1000);

    return () => clearInterval(interval);
  }, []);

  const handleStartRecording = () => {
    setIsRecording(true);
    setSystemStatus('active');
  };

  const handleStopRecording = () => {
    setIsRecording(false);
    setSystemStatus('idle');
  };

  return (
    <InterfaceContainer className={className}>
      {/* Dynamic spacescape background */}
      <SpacescapeBackground
        intensity={backgroundIntensity}
        particleCount={2000}
        nebulaDensity={0.8}
        reactToMouse={true}
        autoRotate={true}
        showNebula={true}
        showStarfield={true}
        showAurora={true}
      />

      {/* Header */}
      <QuantumHeader
        initial={{ y: -100, opacity: 0 }}
        animate={{ y: 0, opacity: 1 }}
        transition={{ duration: 0.8, delay: 0.2 }}
      >
        <Logo>
          <LogoIcon />
          <h1>DuckMuxPeg Matrix</h1>
        </Logo>
        
        <HeaderControls>
          <AdvancedButton
            variant="glass"
            size="sm"
            onClick={() => setBackgroundIntensity(prev => prev === 1 ? 0.3 : 1)}
          >
            {backgroundIntensity === 1 ? 'Reduce Effects' : 'Full Effects'}
          </AdvancedButton>
          
          <AdvancedButton
            variant="neon"
            size="sm"
            glowColor="var(--color-neon-green)"
            onClick={() => setShowMetrics(!showMetrics)}
          >
            Metrics
          </AdvancedButton>
        </HeaderControls>
      </QuantumHeader>

      {/* Central Control Panel */}
      <ControlPanel
        initial={{ scale: 0.8, opacity: 0 }}
        animate={{ scale: 1, opacity: 1 }}
        transition={{ duration: 0.8, delay: 0.5 }}
      >
        <PanelTitle>Advanced Matrix Controls</PanelTitle>
        
        <ButtonGrid>
          <ButtonShowcase>
            <AdvancedButton
              variant="quantum"
              effect="energy"
              energyFlow={true}
              onClick={() => setSelectedTool('transform')}
            >
              Quantum Transform
            </AdvancedButton>
            
            <AdvancedButton
              variant="plasma"
              effect="pulse"
              onClick={() => setSelectedTool('effects')}
            >
              Plasma Effects
            </AdvancedButton>
          </ButtonShowcase>
          
          <ButtonShowcase>
            <AdvancedButton
              variant="hologram"
              effect="matrix"
              onClick={() => setSelectedTool('matrix')}
            >
              Hologram Matrix
            </AdvancedButton>
            
            <AdvancedButton
              variant="neon"
              effect="ripple"
              glowColor="var(--color-neon-pink)"
              onClick={() => setSelectedTool('particle')}
            >
              Neon Particles
            </AdvancedButton>
          </ButtonShowcase>
        </ButtonGrid>

        <ButtonShowcase>
          <AdvancedButton
            variant={isRecording ? "secondary" : "primary"}
            size="lg"
            effect="warp"
            fullWidth
            onClick={isRecording ? handleStopRecording : handleStartRecording}
            icon={isRecording ? "⏹" : "⏺"}
          >
            {isRecording ? 'Stop Recording' : 'Start Recording'}
          </AdvancedButton>
        </ButtonShowcase>
      </ControlPanel>

      {/* Left Side Panel - Matrix Visualization */}
      <AnimatePresence>
        {showMetrics && (
          <SidePanel
            side="left"
            initial={{ x: -350, opacity: 0 }}
            animate={{ x: 0, opacity: 1 }}
            exit={{ x: -350, opacity: 0 }}
            transition={{ duration: 0.6 }}
          >
            <StatusIndicator status={systemStatus}>
              System Status: {systemStatus.toUpperCase()}
            </StatusIndicator>
            
            <MatrixVisualization />
            
            <h3 style={{ marginBottom: 'var(--space-md)', color: 'var(--color-primary)' }}>
              Performance Metrics
            </h3>
            
            <MetricsGrid>
              <MetricCard>
                <div className="value">{metrics.fps.toFixed(0)}</div>
                <div className="label">FPS</div>
              </MetricCard>
              <MetricCard>
                <div className="value">{metrics.cpu.toFixed(0)}%</div>
                <div className="label">CPU</div>
              </MetricCard>
              <MetricCard>
                <div className="value">{metrics.memory.toFixed(0)}%</div>
                <div className="label">Memory</div>
              </MetricCard>
              <MetricCard>
                <div className="value">{metrics.gpu.toFixed(0)}%</div>
                <div className="label">GPU</div>
              </MetricCard>
            </MetricsGrid>
          </SidePanel>
        )}
      </AnimatePresence>

      {/* Right Side Panel - Scene Elements */}
      <SidePanel
        side="right"
        initial={{ x: 350, opacity: 0 }}
        animate={{ x: 0, opacity: 1 }}
        transition={{ duration: 0.6, delay: 0.3 }}
      >
        <h3 style={{ marginBottom: 'var(--space-md)', color: 'var(--color-tertiary)' }}>
          Scene Elements
        </h3>
        
        <div style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-sm)' }}>
          <AdvancedButton
            variant="glass"
            size="sm"
            fullWidth
            effect="energy"
          >
            Video Layer 1
          </AdvancedButton>
          
          <AdvancedButton
            variant="glass"
            size="sm"
            fullWidth
            effect="energy"
          >
            Audio Stream
          </AdvancedButton>
          
          <AdvancedButton
            variant="glass"
            size="sm"
            fullWidth
            effect="energy"
          >
            Overlay Effects
          </AdvancedButton>
          
          <AdvancedButton
            variant="glass"
            size="sm"
            fullWidth
            effect="energy"
          >
            Text Elements
          </AdvancedButton>
        </div>

        <div style={{ marginTop: 'var(--space-xl)' }}>
          <h4 style={{ marginBottom: 'var(--space-md)', color: 'var(--color-quaternary)' }}>
            Matrix Layers
          </h4>
          
          <div style={{ display: 'flex', flexDirection: 'column', gap: 'var(--space-xs)' }}>
            {['Layer 1', 'Layer 2', 'Layer 3'].map((layer, index) => (
              <div
                key={layer}
                style={{
                  padding: 'var(--space-sm)',
                  background: 'rgba(255, 255, 255, 0.05)',
                  border: '1px solid rgba(255, 255, 255, 0.1)',
                  borderRadius: 'var(--radius-sm)',
                  fontSize: '0.9rem',
                  opacity: index === 0 ? 1 : 0.6
                }}
              >
                {layer} {index === 0 && '(Active)'}
              </div>
            ))}
          </div>
        </div>
      </SidePanel>

      {/* Bottom Toolbar */}
      <BottomToolbar
        initial={{ y: 100, opacity: 0 }}
        animate={{ y: 0, opacity: 1 }}
        transition={{ duration: 0.8, delay: 0.7 }}
      >
        {['transform', 'effects', 'matrix', 'particle'].map((tool) => (
          <AdvancedButton
            key={tool}
            variant={selectedTool === tool ? 'primary' : 'glass'}
            size="sm"
            onClick={() => setSelectedTool(tool)}
          >
            {tool.charAt(0).toUpperCase() + tool.slice(1)}
          </AdvancedButton>
                  ))}
        </BottomToolbar>

        {/* Demo Showcase - Toggle with ? button */}
        <DemoShowcase />
      </InterfaceContainer>
    );
  };