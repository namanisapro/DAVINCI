#!/usr/bin/env python3
"""
High-Frequency Trading Market Maker Analytics

This script analyzes simulation results and generates comprehensive visualizations
including PnL curves, trade analysis, and performance metrics.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
import argparse
import sys
from datetime import datetime
import warnings

# Suppress warnings for cleaner output
warnings.filterwarnings('ignore')

# Set style for better-looking plots
plt.style.use('seaborn-v0_8')
sns.set_palette("husl")

class HFTAnalyzer:
    """Main analytics class for HFT simulation results."""
    
    def __init__(self, data_dir="data"):
        self.data_dir = Path(data_dir)
        self.data_dir.mkdir(exist_ok=True)
        
        # Data storage
        self.pnl_data = None
        self.trade_data = None
        self.orderbook_data = None
        
        # Analysis results
        self.performance_metrics = {}
        
    def load_data(self):
        """Load all available data files."""
        print("Loading simulation data...")
        
        # Load PnL data
        pnl_files = list(self.data_dir.glob("*pnl*.csv"))
        if pnl_files:
            self.pnl_data = pd.read_csv(pnl_files[0])
            print(f"Loaded PnL data: {len(self.pnl_data)} records")
        else:
            print("No PnL data found")
            
        # Load trade data
        trade_files = list(self.data_dir.glob("*trade*.csv"))
        if trade_files:
            self.trade_data = pd.read_csv(trade_files[0])
            print(f"Loaded trade data: {len(self.trade_data)} records")
        else:
            print("No trade data found")
            
        # Load order book data
        orderbook_files = list(self.data_dir.glob("*orderbook*.csv"))
        if orderbook_files:
            self.orderbook_data = pd.read_csv(orderbook_files[0])
            print(f"Loaded order book data: {len(self.orderbook_data)} records")
        else:
            print("No order book data found")
            
        if not any([self.pnl_data is not None, self.trade_data is not None, self.orderbook_data is not None]):
            print("No data files found. Please run the simulation first.")
            return False
            
        return True
    
    def preprocess_data(self):
        """Preprocess and clean the loaded data."""
        print("Preprocessing data...")
        
        if self.pnl_data is not None:
            # Convert timestamp to datetime
            if 'Timestamp' in self.pnl_data.columns:
                self.pnl_data['Timestamp'] = pd.to_datetime(self.pnl_data['Timestamp'])
                self.pnl_data.set_index('Timestamp', inplace=True)
            
            # Calculate additional metrics
            self.pnl_data['CumulativePnL'] = self.pnl_data['TotalPnL'].cumsum()
            self.pnl_data['DailyPnL'] = self.pnl_data['DailyPnL'].fillna(0)
            
        if self.trade_data is not None:
            # Convert timestamp to datetime
            if 'Timestamp' in self.trade_data.columns:
                self.trade_data['Timestamp'] = pd.to_datetime(self.trade_data['Timestamp'])
                self.trade_data.set_index('Timestamp', inplace=True)
            
            # Calculate trade metrics
            self.trade_data['TradeValue'] = self.trade_data['Price'] * self.trade_data['Quantity']
            self.trade_data['Side'] = self.trade_data['Side'].map({'BUY': 1, 'SELL': -1})
            
        print("Data preprocessing completed.")
    
    def calculate_performance_metrics(self):
        """Calculate comprehensive performance metrics."""
        print("Calculating performance metrics...")
        
        if self.pnl_data is not None:
            # Basic PnL metrics
            self.performance_metrics['TotalPnL'] = self.pnl_data['TotalPnL'].iloc[-1]
            self.performance_metrics['MaxPnL'] = self.pnl_data['TotalPnL'].max()
            self.performance_metrics['MinPnL'] = self.pnl_data['TotalPnL'].min()
            
            # Drawdown analysis
            cumulative = self.pnl_data['CumulativePnL']
            running_max = cumulative.expanding().max()
            drawdown = (cumulative - running_max) / running_max
            self.performance_metrics['MaxDrawdown'] = drawdown.min()
            
            # Volatility
            returns = self.pnl_data['TotalPnL'].pct_change().dropna()
            self.performance_metrics['Volatility'] = returns.std() * np.sqrt(252)  # Annualized
            
            # Sharpe ratio (assuming 0 risk-free rate)
            if self.performance_metrics['Volatility'] > 0:
                self.performance_metrics['SharpeRatio'] = (returns.mean() * 252) / self.performance_metrics['Volatility']
            else:
                self.performance_metrics['SharpeRatio'] = 0
                
        if self.trade_data is not None:
            # Trade statistics
            self.performance_metrics['TotalTrades'] = len(self.trade_data)
            self.performance_metrics['BuyTrades'] = len(self.trade_data[self.trade_data['Side'] == 1])
            self.performance_metrics['SellTrades'] = len(self.trade_data[self.trade_data['Side'] == -1])
            
            # Volume analysis
            self.performance_metrics['TotalVolume'] = self.trade_data['Quantity'].sum()
            self.performance_metrics['AvgTradeSize'] = self.trade_data['Quantity'].mean()
            
            # Price analysis
            self.performance_metrics['AvgTradePrice'] = self.trade_data['Price'].mean()
            self.performance_metrics['PriceRange'] = self.trade_data['Price'].max() - self.trade_data['Price'].min()
            
        print("Performance metrics calculated.")
    
    def plot_pnl_curve(self):
        """Plot the PnL curve over time."""
        if self.pnl_data is None:
            print("No PnL data available for plotting.")
            return
            
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))
        
        # PnL over time
        ax1.plot(self.pnl_data.index, self.pnl_data['TotalPnL'], linewidth=2, color='blue', alpha=0.8)
        ax1.set_title('PnL Over Time', fontsize=14, fontweight='bold')
        ax1.set_ylabel('PnL ($)', fontsize=12)
        ax1.grid(True, alpha=0.3)
        ax1.axhline(y=0, color='red', linestyle='--', alpha=0.7)
        
        # Cumulative PnL
        ax2.plot(self.pnl_data.index, self.pnl_data['CumulativePnL'], linewidth=2, color='green', alpha=0.8)
        ax2.set_title('Cumulative PnL', fontsize=14, fontweight='bold')
        ax2.set_xlabel('Time', fontsize=12)
        ax2.set_ylabel('Cumulative PnL ($)', fontsize=12)
        ax2.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(self.data_dir / 'pnl_curve.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def plot_trade_analysis(self):
        """Plot trade analysis charts."""
        if self.trade_data is None:
            print("No trade data available for plotting.")
            return
            
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
        
        # Trade volume over time
        trade_data_resampled = self.trade_data.resample('1T')['Quantity'].sum()
        ax1.bar(trade_data_resampled.index, trade_data_resampled.values, alpha=0.7, color='skyblue')
        ax1.set_title('Trade Volume Over Time', fontsize=12, fontweight='bold')
        ax1.set_ylabel('Volume', fontsize=10)
        ax1.tick_params(axis='x', rotation=45)
        
        # Price distribution
        ax2.hist(self.trade_data['Price'], bins=30, alpha=0.7, color='lightcoral', edgecolor='black')
        ax2.set_title('Trade Price Distribution', fontsize=12, fontweight='bold')
        ax2.set_xlabel('Price ($)', fontsize=10)
        ax2.set_ylabel('Frequency', fontsize=10)
        
        # Trade size distribution
        ax3.hist(self.trade_data['Quantity'], bins=30, alpha=0.7, color='lightgreen', edgecolor='black')
        ax3.set_title('Trade Size Distribution', fontsize=12, fontweight='bold')
        ax3.set_xlabel('Quantity', fontsize=10)
        ax3.set_ylabel('Frequency', fontsize=10)
        
        # Buy vs Sell ratio
        side_counts = self.trade_data['Side'].value_counts()
        ax4.pie(side_counts.values, labels=['Sell', 'Buy'], autopct='%1.1f%%', 
                colors=['lightcoral', 'lightblue'], startangle=90)
        ax4.set_title('Buy vs Sell Ratio', fontsize=12, fontweight='bold')
        
        plt.tight_layout()
        plt.savefig(self.data_dir / 'trade_analysis.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def plot_performance_dashboard(self):
        """Create a comprehensive performance dashboard."""
        if not self.performance_metrics:
            print("No performance metrics available for dashboard.")
            return
            
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 10))
        
        # Key metrics table
        metrics_text = f"""
        Total PnL: ${self.performance_metrics.get('TotalPnL', 0):,.2f}
        Max Drawdown: {self.performance_metrics.get('MaxDrawdown', 0)*100:.2f}%
        Sharpe Ratio: {self.performance_metrics.get('SharpeRatio', 0):.3f}
        Volatility: {self.performance_metrics.get('Volatility', 0)*100:.2f}%
        Total Trades: {self.performance_metrics.get('TotalTrades', 0):,}
        """
        
        ax1.text(0.1, 0.5, metrics_text, transform=ax1.transAxes, fontsize=12, 
                verticalalignment='center', fontfamily='monospace',
                bbox=dict(boxstyle="round,pad=0.3", facecolor="lightblue", alpha=0.5))
        ax1.set_title('Performance Metrics', fontsize=14, fontweight='bold')
        ax1.axis('off')
        
        # PnL distribution
        if self.pnl_data is not None:
            ax2.hist(self.pnl_data['TotalPnL'], bins=30, alpha=0.7, color='lightgreen', edgecolor='black')
            ax2.set_title('PnL Distribution', fontsize=12, fontweight='bold')
            ax2.set_xlabel('PnL ($)', fontsize=10)
            ax2.set_ylabel('Frequency', fontsize=10)
            ax2.axvline(x=0, color='red', linestyle='--', alpha=0.7)
        
        # Trade analysis
        if self.trade_data is not None:
            # Trade timing analysis
            trade_data_resampled = self.trade_data.resample('1T').size()
            ax3.plot(trade_data_resampled.index, trade_data_resampled.values, 
                    linewidth=2, color='purple', alpha=0.8)
            ax3.set_title('Trade Frequency Over Time', fontsize=12, fontweight='bold')
            ax3.set_xlabel('Time', fontsize=10)
            ax3.set_ylabel('Trades per Minute', fontsize=10)
            ax3.tick_params(axis='x', rotation=45)
        
        # Risk metrics
        risk_metrics = ['MaxDrawdown', 'Volatility', 'SharpeRatio']
        risk_values = [self.performance_metrics.get(metric, 0) for metric in risk_metrics]
        
        # Normalize values for better visualization
        if max(risk_values) > 0:
            risk_values = [v / max(risk_values) for v in risk_values]
        
        bars = ax4.bar(risk_metrics, risk_values, color=['red', 'orange', 'green'], alpha=0.7)
        ax4.set_title('Risk Metrics (Normalized)', fontsize=12, fontweight='bold')
        ax4.set_ylabel('Normalized Value', fontsize=10)
        ax4.tick_params(axis='x', rotation=45)
        
        # Add value labels on bars
        for bar, value in zip(bars, risk_values):
            height = bar.get_height()
            ax4.text(bar.get_x() + bar.get_width()/2., height,
                    f'{value:.3f}', ha='center', va='bottom')
        
        plt.tight_layout()
        plt.savefig(self.data_dir / 'performance_dashboard.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def generate_report(self):
        """Generate a comprehensive analysis report."""
        if not self.performance_metrics:
            print("No performance metrics available for report generation.")
            return
            
        report_file = self.data_dir / f'analysis_report_{datetime.now().strftime("%Y%m%d_%H%M%S")}.txt'
        
        with open(report_file, 'w') as f:
            f.write("=" * 60 + "\n")
            f.write("HIGH-FREQUENCY TRADING MARKET MAKER ANALYSIS REPORT\n")
            f.write("=" * 60 + "\n\n")
            f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
            
            f.write("PERFORMANCE METRICS\n")
            f.write("-" * 30 + "\n")
            for key, value in self.performance_metrics.items():
                if isinstance(value, float):
                    f.write(f"{key}: {value:,.4f}\n")
                else:
                    f.write(f"{key}: {value:,}\n")
            
            f.write("\n" + "=" * 60 + "\n")
            f.write("END OF REPORT\n")
            f.write("=" * 60 + "\n")
        
        print(f"Analysis report generated: {report_file}")
    
    def run_analysis(self):
        """Run the complete analysis pipeline."""
        print("Starting HFT Market Maker Analysis...")
        print("=" * 50)
        
        # Load and process data
        if not self.load_data():
            return
            
        self.preprocess_data()
        self.calculate_performance_metrics()
        
        # Generate visualizations
        print("\nGenerating visualizations...")
        self.plot_pnl_curve()
        self.plot_trade_analysis()
        self.plot_performance_dashboard()
        
        # Generate report
        print("\nGenerating analysis report...")
        self.generate_report()
        
        # Print summary
        print("\n" + "=" * 50)
        print("ANALYSIS COMPLETED")
        print("=" * 50)
        print(f"Total PnL: ${self.performance_metrics.get('TotalPnL', 0):,.2f}")
        print(f"Max Drawdown: {self.performance_metrics.get('MaxDrawdown', 0)*100:.2f}%")
        print(f"Sharpe Ratio: {self.performance_metrics.get('SharpeRatio', 0):.3f}")
        print(f"Total Trades: {self.performance_metrics.get('TotalTrades', 0):,}")
        print(f"Output directory: {self.data_dir}")

def main():
    """Main function to run the analysis."""
    parser = argparse.ArgumentParser(description='HFT Market Maker Analytics')
    parser.add_argument('--data-dir', default='data', help='Directory containing simulation data')
    parser.add_argument('--output-dir', default='data', help='Output directory for analysis results')
    
    args = parser.parse_args()
    
    # Create analyzer and run analysis
    analyzer = HFTAnalyzer(data_dir=args.data_dir)
    analyzer.run_analysis()

if __name__ == "__main__":
    main()
