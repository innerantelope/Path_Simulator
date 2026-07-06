/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  theme: {
    extend: {
      colors: {
        ink: '#050816',
        panel: '#0d1224',
        line: '#26304f',
        accent: '#23c7d9',
        success: '#2ddf8f',
        warning: '#f8c14a',
        danger: '#ff5c7a'
      },
      boxShadow: {
        glow: '0 0 36px rgba(35, 199, 217, 0.18)'
      }
    }
  },
  plugins: []
};
