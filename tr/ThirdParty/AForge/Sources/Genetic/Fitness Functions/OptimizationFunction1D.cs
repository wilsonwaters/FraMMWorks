// AForge Genetic Library
//
// Copyright � Andrew Kirillov, 2006
// andrew.kirillov@gmail.com
//

namespace AForge.Genetic
{
	using System;
	using AForge;

	/// <summary>Base class for one dimensional function optimizations</summary>
	/// 
	/// <remarks><para>The class is aimed to be used for one dimensional function
	/// optimization problems. It implements all methods of <see cref="IFitnessFunction"/>
	/// interface and requires overriding only one method -
	/// <see cref="OptimizationFunction"/>, which represents the
    /// function to optimize.</para>
    /// <para><note>The optimization function should be greater
    /// then 0 on the specified optimization range.</note></para>
    /// <para>The class works only with binary chromosomes (<see cref="BinaryChromosome"/>).</para>
    /// </remarks>
	/// 
	/// <example>The following sample illustrates the usage of <c>OptimizationFunction1D</c> class:
	/// <code>
	/// // define optimization function
	/// public class UserFunction : OptimizationFunction1D
	/// {
	///		public UserFunction( ) :
	///			base( new DoubleRange( 0, 255 ) ) { }
	///
	/// 	public override double OptimizationFunction( double x )
	///		{
	///			return Math.Cos( x / 23 ) * Math.Sin( x / 50 ) + 2;
	///		}
	/// }
	/// ...
	/// // create genetic population
	/// Population population = new Population( 40,
	///		new BinaryChromosome( 32 ),
	///		new UserFunction( ),
	///		new EliteSelection( ) );
	///	// run one epoch of the population
	///	population.RunEpoch( );
	/// </code>
	/// </example>
	///
	public abstract class OptimizationFunction1D : IFitnessFunction
	{
		/// <summary>
		/// Optimization modes
		/// </summary>
		///
		/// <remarks>The enumeration defines optimization modes for
		/// the one dimensional function optimization.</remarks> 
		///
		public enum Modes
		{
			/// <summary>
			/// Search for function's maximum value
			/// </summary>
			Maximization,
			/// <summary>
			/// Search for function's minimum value
			/// </summary>
			Minimization
		}
		
		// optimization range
		private DoubleRange	range = new DoubleRange( 0, 1 );
		// optimization mode
		private Modes mode = Modes.Maximization;

		/// <summary>
		/// Optimization range
		/// </summary>
		/// 
		/// <remarks>Defines function's input range. The function's extreme will
		/// be searched in this range only.
		/// </remarks>
		/// 
		public DoubleRange Range
		{
			get { return range; }
			set { range = value; }
		}

		/// <summary>
		/// Optimization mode
		/// </summary>
		///
		/// <remarks>Defines optimization mode - what kind of extreme to search</remarks> 
		///
		public Modes Mode
		{
			get { return mode; }
			set { mode = value; }
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="OptimizationFunction1D"/> class
		/// </summary>
		///
		/// <param name="range">Specifies range for optimization</param>
		///
		public OptimizationFunction1D( DoubleRange range )
		{
			this.range = range;
		}

		/// <summary>
		/// Evaluates chromosome
		/// </summary>
		/// 
		/// <param name="chromosome">Chromosome to evaluate</param>
		/// 
		/// <returns>Returns chromosome's fitness value</returns>
		///
		/// <remarks>The method calculates fitness value of the specified
		/// chromosome.</remarks>
		///
		public double Evaluate( IChromosome chromosome )
		{
			double functionValue = OptimizationFunction( TranslateNative( chromosome ) );
			// fitness value
			return ( mode == Modes.Maximization ) ? functionValue : 1 / functionValue;
		}

		/// <summary>
		/// Translates genotype to phenotype 
		/// </summary>
		/// 
		/// <param name="chromosome">Chromosome, which genoteype should be
		/// translated to phenotype</param>
		///
		/// <returns>Returns chromosome's fenotype - the actual solution
		/// encoded by the chromosome</returns> 
		/// 
		/// <remarks>The method returns object, which represents function's
		/// input point encoded by the specified chromosome. The object's type is
		/// double.</remarks>
		///
		public object Translate( IChromosome chromosome )
		{
			return TranslateNative( chromosome );
		}

		/// <summary>
		/// Translates genotype to phenotype 
		/// </summary>
		/// 
		/// <param name="chromosome">Chromosome, which genoteype should be
		/// translated to phenotype</param>
		///
		/// <returns>Returns chromosome's fenotype - the actual solution
		/// encoded by the chromosome</returns> 
		/// 
		/// <remarks>The method returns double value, which represents function's
		/// input point encoded by the specified chromosome.</remarks>
		///
		public double TranslateNative( IChromosome chromosome )
		{
			// get chromosome's value and max value
			double val = ((BinaryChromosome) chromosome).Value;
			double max = ((BinaryChromosome) chromosome).MaxValue;

			// translate to optimization's funtion space
			return val * range.Length / max + range.Min;
		}

		/// <summary>
		/// Function to optimize
		/// </summary>
		///
		/// <param name="x">Function input value</param>
		/// 
		/// <returns>Returns function output value</returns>
		/// 
		/// <remarks>The method should be overloaded by inherited class to
		/// specify the optimization function.</remarks>
		///
		public abstract double OptimizationFunction( double x );
	}
}
