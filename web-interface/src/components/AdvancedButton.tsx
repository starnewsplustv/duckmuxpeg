import React, { useState, useRef, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import styled, { css, keyframes } from 'styled-components';

// Advanced Button Types
export type ButtonVariant = 'primary' | 'secondary' | 'neon' | 'glass' | 'quantum' | 'plasma' | 'hologram';
export type ButtonSize = 'xs' | 'sm' | 'md' | 'lg' | 'xl';
export type ButtonEffect = 'ripple' | 'glow' | 'pulse' | 'matrix' | 'energy' | 'warp' | 'none';

interface AdvancedButtonProps {
  children: React.ReactNode;
  variant?: ButtonVariant;
  size?: ButtonSize;
  effect?: ButtonEffect;
  disabled?: boolean;
  loading?: boolean;
  fullWidth?: boolean;
  glowColor?: string;
  particleCount?: number;
  energyFlow?: boolean;
  onClick?: () => void;
  className?: string;
  icon?: React.ReactNode;
  rightIcon?: React.ReactNode;
}

// Keyframe Animations
const energyFlow = keyframes`
  0% { background-position: 0% 50%; }
  50% { background-position: 100% 50%; }
  100% { background-position: 0% 50%; }
`;

const quantumShift = keyframes`
  0%, 100% { transform: translateX(0) scale(1); }
  25% { transform: translateX(-1px) scale(1.02); }
  50% { transform: translateX(1px) scale(0.98); }
  75% { transform: translateX(-0.5px) scale(1.01); }
`;

const matrixGlitch = keyframes`
  0% { text-shadow: 0 0 0 var(--color-neon-blue); }
  10% { text-shadow: 2px 0 0 var(--color-neon-blue), -2px 0 0 var(--color-neon-pink); }
  20% { text-shadow: 0 0 0 var(--color-neon-blue); }
  30% { text-shadow: 1px 0 0 var(--color-neon-green), -1px 0 0 var(--color-neon-purple); }
  40% { text-shadow: 0 0 0 var(--color-neon-blue); }
  100% { text-shadow: 0 0 0 var(--color-neon-blue); }
`;

const hologramFlicker = keyframes`
  0%, 100% { opacity: 1; }
  50% { opacity: 0.8; }
  75% { opacity: 0.9; }
`;

// Styled Components
const ButtonWrapper = styled(motion.button)<AdvancedButtonProps>`
  position: relative;
  border: none;
  cursor: pointer;
  font-family: var(--font-primary);
  font-weight: 600;
  text-decoration: none;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: var(--space-sm);
  transition: all var(--timing-normal) var(--ease-smooth);
  overflow: hidden;
  border-radius: var(--radius-md);
  outline: none;
  
  ${props => props.fullWidth && css`
    width: 100%;
  `}
  
  ${props => props.disabled && css`
    opacity: 0.5;
    cursor: not-allowed;
    pointer-events: none;
  `}
  
  // Size Variants
  ${props => {
    switch (props.size) {
      case 'xs':
        return css`
          padding: var(--space-xs) var(--space-sm);
          font-size: 0.75rem;
          min-height: 1.5rem;
        `;
      case 'sm':
        return css`
          padding: var(--space-sm) var(--space-md);
          font-size: 0.875rem;
          min-height: 2rem;
        `;
      case 'md':
        return css`
          padding: var(--space-md) var(--space-lg);
          font-size: 1rem;
          min-height: 2.5rem;
        `;
      case 'lg':
        return css`
          padding: var(--space-lg) var(--space-xl);
          font-size: 1.125rem;
          min-height: 3rem;
        `;
      case 'xl':
        return css`
          padding: var(--space-xl) var(--space-2xl);
          font-size: 1.25rem;
          min-height: 3.5rem;
        `;
      default:
        return css`
          padding: var(--space-md) var(--space-lg);
          font-size: 1rem;
          min-height: 2.5rem;
        `;
    }
  }}
  
  // Variant Styles
  ${props => {
    switch (props.variant) {
      case 'primary':
        return css`
          background: linear-gradient(135deg, var(--color-primary), var(--color-primary-dark));
          color: white;
          box-shadow: var(--shadow-glow);
          
          &:hover {
            background: linear-gradient(135deg, var(--color-primary-light), var(--color-primary));
            transform: translateY(-2px);
            box-shadow: 0 0 30px rgba(0, 212, 255, 0.5);
          }
        `;
        
      case 'secondary':
        return css`
          background: linear-gradient(135deg, var(--color-secondary), #e55555);
          color: white;
          
          &:hover {
            background: linear-gradient(135deg, #ff8080, var(--color-secondary));
            transform: translateY(-2px);
          }
        `;
        
      case 'neon':
        return css`
          background: transparent;
          color: ${props.glowColor || 'var(--color-neon-blue)'};
          border: 2px solid ${props.glowColor || 'var(--color-neon-blue)'};
          text-shadow: 0 0 10px ${props.glowColor || 'var(--color-neon-blue)'};
          box-shadow: 0 0 20px ${props.glowColor || 'var(--color-neon-blue)'};
          
          &:hover {
            background: ${props.glowColor || 'var(--color-neon-blue)'};
            color: var(--color-space-deep);
            text-shadow: none;
            box-shadow: 0 0 40px ${props.glowColor || 'var(--color-neon-blue)'};
            transform: scale(1.05);
          }
        `;
        
      case 'glass':
        return css`
          background: rgba(255, 255, 255, 0.05);
          backdrop-filter: blur(20px);
          border: 1px solid rgba(255, 255, 255, 0.1);
          color: white;
          
          &:hover {
            background: rgba(255, 255, 255, 0.1);
            border-color: rgba(255, 255, 255, 0.2);
            transform: translateY(-1px);
          }
        `;
        
      case 'quantum':
        return css`
          background: linear-gradient(45deg, var(--color-neon-purple), var(--color-neon-blue), var(--color-neon-green));
          background-size: 300% 300%;
          animation: ${energyFlow} 3s ease infinite;
          color: white;
          text-shadow: 0 0 10px rgba(255, 255, 255, 0.8);
          
          &:hover {
            animation: ${quantumShift} 0.3s ease;
            box-shadow: 0 0 30px var(--color-neon-purple);
          }
        `;
        
      case 'plasma':
        return css`
          background: radial-gradient(circle at 50% 50%, var(--color-neon-pink), var(--color-neon-purple), var(--color-space-dark));
          color: white;
          position: relative;
          overflow: hidden;
          
          &::before {
            content: '';
            position: absolute;
            top: -50%;
            left: -50%;
            width: 200%;
            height: 200%;
            background: conic-gradient(from 0deg, var(--color-neon-blue), var(--color-neon-green), var(--color-neon-orange), var(--color-neon-pink), var(--color-neon-blue));
            animation: ${energyFlow} 4s linear infinite;
            opacity: 0.7;
            z-index: -1;
          }
          
          &:hover {
            transform: scale(1.02);
            filter: brightness(1.2);
          }
        `;
        
      case 'hologram':
        return css`
          background: linear-gradient(135deg, transparent, rgba(0, 255, 255, 0.1), transparent);
          border: 1px solid var(--color-neon-blue);
          color: var(--color-neon-blue);
          animation: ${hologramFlicker} 2s ease-in-out infinite;
          
          &:hover {
            background: linear-gradient(135deg, rgba(0, 255, 255, 0.1), rgba(0, 255, 255, 0.2), rgba(0, 255, 255, 0.1));
            text-shadow: 0 0 15px var(--color-neon-blue);
          }
        `;
        
      default:
        return css`
          background: var(--color-space-medium);
          color: white;
          border: 1px solid var(--color-space-light);
          
          &:hover {
            background: var(--color-space-light);
            transform: translateY(-1px);
          }
        `;
    }
  }}
  
  // Effect Animations
  ${props => {
    switch (props.effect) {
      case 'pulse':
        return css`
          animation: pulseGlow 2s ease-in-out infinite alternate;
        `;
      case 'matrix':
        return css`
          &:hover {
            animation: ${matrixGlitch} 0.5s ease;
          }
        `;
      case 'energy':
        return css`
          &::after {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.3), transparent);
            transition: left var(--timing-slow) var(--ease-smooth);
          }
          
          &:hover::after {
            left: 100%;
          }
        `;
      case 'warp':
        return css`
          &:active {
            transform: perspective(1000px) rotateX(10deg) scale(0.95);
          }
        `;
      default:
        return '';
    }
  }}
`;

const RippleEffect = styled.div`
  position: absolute;
  border-radius: 50%;
  background: rgba(255, 255, 255, 0.6);
  transform: scale(0);
  animation: ripple 0.6s linear;
  pointer-events: none;
  
  @keyframes ripple {
    to {
      transform: scale(4);
      opacity: 0;
    }
  }
`;

const LoadingSpinner = styled.div`
  width: 1em;
  height: 1em;
  border: 2px solid transparent;
  border-top: 2px solid currentColor;
  border-radius: 50%;
  animation: spin 1s linear infinite;
  
  @keyframes spin {
    to { transform: rotate(360deg); }
  }
`;

const ParticleContainer = styled.div`
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
  overflow: hidden;
`;

const Particle = styled.div<{ delay: number; duration: number; x: number; y: number }>`
  position: absolute;
  width: 2px;
  height: 2px;
  background: currentColor;
  border-radius: 50%;
  left: ${props => props.x}%;
  top: ${props => props.y}%;
  animation: particleFloat ${props => props.duration}s ease-in-out ${props => props.delay}s infinite;
  
  @keyframes particleFloat {
    0%, 100% { transform: translateY(0) scale(0); opacity: 0; }
    10% { transform: translateY(-10px) scale(1); opacity: 1; }
    90% { transform: translateY(-20px) scale(0.5); opacity: 0.5; }
  }
`;

export const AdvancedButton: React.FC<AdvancedButtonProps> = ({
  children,
  variant = 'primary',
  size = 'md',
  effect = 'none',
  disabled = false,
  loading = false,
  fullWidth = false,
  glowColor,
  particleCount = 5,
  energyFlow = false,
  onClick,
  className,
  icon,
  rightIcon,
  ...props
}) => {
  const [ripples, setRipples] = useState<Array<{ id: number; x: number; y: number; size: number }>>([]);
  const [isHovered, setIsHovered] = useState(false);
  const buttonRef = useRef<HTMLButtonElement>(null);

  const handleClick = (e: React.MouseEvent<HTMLButtonElement>) => {
    if (disabled || loading) return;
    
    if (effect === 'ripple') {
      const rect = buttonRef.current?.getBoundingClientRect();
      if (rect) {
        const size = Math.max(rect.width, rect.height);
        const x = e.clientX - rect.left - size / 2;
        const y = e.clientY - rect.top - size / 2;
        
        const newRipple = {
          id: Date.now(),
          x,
          y,
          size,
        };
        
        setRipples(prev => [...prev, newRipple]);
        
        setTimeout(() => {
          setRipples(prev => prev.filter(ripple => ripple.id !== newRipple.id));
        }, 600);
      }
    }
    
    onClick?.();
  };

  const generateParticles = () => {
    return Array.from({ length: particleCount }, (_, i) => (
      <Particle
        key={i}
        delay={i * 0.1}
        duration={2 + Math.random() * 2}
        x={Math.random() * 100}
        y={Math.random() * 100}
      />
    ));
  };

  return (
    <ButtonWrapper
      ref={buttonRef}
      variant={variant}
      size={size}
      effect={effect}
      disabled={disabled}
      fullWidth={fullWidth}
      glowColor={glowColor}
      onClick={handleClick}
      onMouseEnter={() => setIsHovered(true)}
      onMouseLeave={() => setIsHovered(false)}
      className={className}
      whileHover={{ scale: variant === 'quantum' ? 1.02 : 1.01 }}
      whileTap={{ scale: 0.98 }}
      transition={{ type: "spring", stiffness: 400, damping: 17 }}
      {...props}
    >
      {/* Particle Effects */}
      {(effect === 'energy' || energyFlow) && isHovered && (
        <ParticleContainer>
          {generateParticles()}
        </ParticleContainer>
      )}
      
      {/* Ripple Effects */}
      {effect === 'ripple' && ripples.map(ripple => (
        <RippleEffect
          key={ripple.id}
          style={{
            left: ripple.x,
            top: ripple.y,
            width: ripple.size,
            height: ripple.size,
          }}
        />
      ))}
      
      {/* Button Content */}
      <AnimatePresence mode="wait">
        {loading ? (
          <motion.div
            key="loading"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-sm)' }}
          >
            <LoadingSpinner />
            Loading...
          </motion.div>
        ) : (
          <motion.div
            key="content"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            style={{ display: 'flex', alignItems: 'center', gap: 'var(--space-sm)' }}
          >
            {icon}
            {children}
            {rightIcon}
          </motion.div>
        )}
      </AnimatePresence>
    </ButtonWrapper>
  );
};