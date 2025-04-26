"use client"

import { useState } from "react"
import { ScaleVisualization } from "@/components/scale-visualization"
import { NutritionFacts } from "@/components/nutrition-facts"
import { FoodImageUpload } from "@/components/food-image-upload"
import { useFoodData } from "@/hooks/use-food-data"

export default function Home() {
  const { foodData, isDataValid, loading: foodDataLoading } = useFoodData()
  const [isAnalyzing, setIsAnalyzing] = useState(false)

  const handleAnalysisStart = () => {
    setIsAnalyzing(true)
  }

  const handleAnalysisComplete = () => {
    setIsAnalyzing(false)
  }

  return (
      <div
  className="min-h-screen bg-gray-900 py-8 bg-cover bg-center"
  style={{ backgroundImage: "url('/bg.png')" }}
>

      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
          {/* Left side: Nutrition facts */}
          <div className="space-y-6">
            <div className={`transition-opacity duration-300 ${isAnalyzing ? "opacity-50" : "opacity-100"}`}>
              <NutritionFacts
                nutritionData={isDataValid ? foodData : null}
                isStale={foodData && !isDataValid}
                isLoading={foodDataLoading}
              />
            </div>
          </div>

          {/* Right side: Scale visualization */}
          <div className="space-y-6">
            <div className="p-6 rounded-lg h-[600px] flex items-center justify-center bg-black/50 backdrop-blur-lg shadow-xl">
              <ScaleVisualization />
            </div>
          </div>
          <FoodImageUpload onAnalysisStart={handleAnalysisStart} onAnalysisComplete={handleAnalysisComplete} />

        </div>
      </main>
    </div>
  )
}
