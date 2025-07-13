import React, { useState } from 'react';
import { motion } from 'framer-motion';
import styled from 'styled-components';
import { AdvancedButton } from './AdvancedButton';

const ShowcaseContainer = styled(motion.div)`
  position: fixed;
  top: 20px;
  right: 20px;
  width: 350px;
  max-height: 80vh;
  overflow-y: auto;
  padding: var(--space-xl);
  background: rgba(0, 0, 0, 0.8);
  backdrop-filter: blur(30px);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: var(--radius-xl);
  z-index: 1000;
`;

const ShowcaseTitle = styled.h2`
  color: var(--color-primary);
  margin-bottom: var(--space-lg);
  text-align: center;
  font-size: 1.2rem;
`;

const VariantGroup = styled.div`
  margin-bottom: var(--space-lg);
  
  h3 {
    color: var(--color-tertiary);
    font-size: 1rem;
    margin-bottom: var(--space-md);
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }
`;

const ButtonRow = styled.div`
  display: flex;
  flex-direction: column;
  gap: var(--space-sm);
  margin-bottom: var(--space-md);
`;

const ButtonDemo = styled.div`
  display: flex;
  align-items: center;
  gap: var(--space-sm);
  padding: var(--space-sm);
  background: rgba(255, 255, 255, 0.02);
  border-radius: var(--radius-md);
  
  .button-container {
    flex: 1;
  }
  
  .button-label {
    font-size: 0.8rem;
    color: rgba(255, 255, 255, 0.6);
    min-width: 60px;
  }
`;

const ToggleButton = styled.button`
  position: absolute;
  top: 10px;
  right: 10px;
  width: 30px;
  height: 30px;
  background: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 50%;
  color: white;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 12px;
  
  &:hover {
    background: rgba(255, 255, 255, 0.2);
  }
`;

export const DemoShowcase: React.FC = () => {
  const [isVisible, setIsVisible] = useState(true);

  if (!isVisible) {
    return (
      <ToggleButton
        onClick={() => setIsVisible(true)}
        style={{ position: 'fixed', top: '20px', right: '20px', zIndex: 1000 }}
      >
        ?
      </ToggleButton>
    );
  }

  return (
    <ShowcaseContainer
      initial={{ x: 400, opacity: 0 }}
      animate={{ x: 0, opacity: 1 }}
      transition={{ duration: 0.5 }}
    >
      <ToggleButton onClick={() => setIsVisible(false)}>
        ×
      </ToggleButton>
      
      <ShowcaseTitle>🎮 Button Showcase</ShowcaseTitle>

      <VariantGroup>
        <h3>🌟 Standard Variants</h3>
        <ButtonRow>
          <ButtonDemo>
            <div className="button-label">Primary</div>
            <div className="button-container">
              <AdvancedButton variant="primary" size="sm" fullWidth>
                Primary Button
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Secondary</div>
            <div className="button-container">
              <AdvancedButton variant="secondary" size="sm" fullWidth>
                Secondary Button
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Glass</div>
            <div className="button-container">
              <AdvancedButton variant="glass" size="sm" fullWidth>
                Glass Morphism
              </AdvancedButton>
            </div>
          </ButtonDemo>
        </ButtonRow>
      </VariantGroup>

      <VariantGroup>
        <h3>⚡ Neon Effects</h3>
        <ButtonRow>
          <ButtonDemo>
            <div className="button-label">Cyan</div>
            <div className="button-container">
              <AdvancedButton 
                variant="neon" 
                size="sm" 
                fullWidth
                glowColor="var(--color-neon-blue)"
              >
                Neon Cyan
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Pink</div>
            <div className="button-container">
              <AdvancedButton 
                variant="neon" 
                size="sm" 
                fullWidth
                glowColor="var(--color-neon-pink)"
              >
                Neon Pink
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Green</div>
            <div className="button-container">
              <AdvancedButton 
                variant="neon" 
                size="sm" 
                fullWidth
                glowColor="var(--color-neon-green)"
              >
                Neon Green
              </AdvancedButton>
            </div>
          </ButtonDemo>
        </ButtonRow>
      </VariantGroup>

      <VariantGroup>
        <h3>🔮 Advanced Variants</h3>
        <ButtonRow>
          <ButtonDemo>
            <div className="button-label">Quantum</div>
            <div className="button-container">
              <AdvancedButton 
                variant="quantum" 
                size="sm" 
                fullWidth
                energyFlow={true}
              >
                Quantum Flow
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Plasma</div>
            <div className="button-container">
              <AdvancedButton variant="plasma" size="sm" fullWidth>
                Plasma Energy
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Hologram</div>
            <div className="button-container">
              <AdvancedButton variant="hologram" size="sm" fullWidth>
                Holographic
              </AdvancedButton>
            </div>
          </ButtonDemo>
        </ButtonRow>
      </VariantGroup>

      <VariantGroup>
        <h3>🎭 Interactive Effects</h3>
        <ButtonRow>
          <ButtonDemo>
            <div className="button-label">Ripple</div>
            <div className="button-container">
              <AdvancedButton 
                variant="primary" 
                effect="ripple" 
                size="sm" 
                fullWidth
              >
                Click for Ripple
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Energy</div>
            <div className="button-container">
              <AdvancedButton 
                variant="neon" 
                effect="energy" 
                size="sm" 
                fullWidth
                glowColor="var(--color-neon-orange)"
                particleCount={8}
              >
                Energy Particles
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Matrix</div>
            <div className="button-container">
              <AdvancedButton 
                variant="glass" 
                effect="matrix" 
                size="sm" 
                fullWidth
              >
                Matrix Glitch
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Pulse</div>
            <div className="button-container">
              <AdvancedButton 
                variant="secondary" 
                effect="pulse" 
                size="sm" 
                fullWidth
              >
                Pulse Glow
              </AdvancedButton>
            </div>
          </ButtonDemo>
        </ButtonRow>
      </VariantGroup>

      <VariantGroup>
        <h3>📏 Size Variants</h3>
        <ButtonRow>
          <ButtonDemo>
            <div className="button-label">XS</div>
            <div className="button-container">
              <AdvancedButton variant="primary" size="xs" fullWidth>
                Extra Small
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">SM</div>
            <div className="button-container">
              <AdvancedButton variant="primary" size="sm" fullWidth>
                Small
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">MD</div>
            <div className="button-container">
              <AdvancedButton variant="primary" size="md" fullWidth>
                Medium
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">LG</div>
            <div className="button-container">
              <AdvancedButton variant="primary" size="lg" fullWidth>
                Large
              </AdvancedButton>
            </div>
          </ButtonDemo>
        </ButtonRow>
      </VariantGroup>

      <VariantGroup>
        <h3>🔧 Special States</h3>
        <ButtonRow>
          <ButtonDemo>
            <div className="button-label">Loading</div>
            <div className="button-container">
              <AdvancedButton 
                variant="quantum" 
                size="sm" 
                fullWidth
                loading={true}
              >
                Processing...
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Disabled</div>
            <div className="button-container">
              <AdvancedButton 
                variant="glass" 
                size="sm" 
                fullWidth
                disabled={true}
              >
                Disabled State
              </AdvancedButton>
            </div>
          </ButtonDemo>
          
          <ButtonDemo>
            <div className="button-label">Icon</div>
            <div className="button-container">
              <AdvancedButton 
                variant="neon" 
                size="sm" 
                fullWidth
                icon="🚀"
                rightIcon="⭐"
                glowColor="var(--color-neon-purple)"
              >
                With Icons
              </AdvancedButton>
            </div>
          </ButtonDemo>
        </ButtonRow>
      </VariantGroup>
    </ShowcaseContainer>
  );
};