"use client"

import { useState } from "react"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Progress } from "@/components/ui/progress"
import { useScaleWeight } from "@/hooks/use-scale-weight"
import { Flame, Wheat, Cookie, Salad, Beef, Droplet, Apple, Sparkles, AlertTriangle, Clock } from "lucide-react"
import { motion, AnimatePresence } from "framer-motion"

type NutritionData = {
  food: string
  nutritional_facts_per_gram: {
    calories: number
    carbohydrates: {
      total: number
      sugars: number
      dietary_fiber: number
    }
    protein: number
    fat: number
    vitamin_c: string
    potassium_mg: number
    water_content: string
  }
  timestamp: number
}

interface NutritionFactsProps {
  nutritionData: NutritionData | null
  isStale?: boolean
  isLoading?: boolean
}

export function NutritionFacts({ nutritionData, isStale = false, isLoading = false }: NutritionFactsProps) {
  const { weight } = useScaleWeight()
  const [expanded, setExpanded] = useState(true)



  if (isLoading) {
    return (
      <Card className="w-full h-full bg-black/50 backdrop-blur-lg shadow-xl">
        <CardHeader className="pb-3">
          <CardTitle className="text-gray-100 flex items-center gap-2">
            <Sparkles className="h-5 w-5" />
            <span>Nutrition Facts</span>
          </CardTitle>
        </CardHeader>
        <CardContent className="flex flex-col items-center justify-center h-64 pt-8">
          <motion.div
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            transition={{ duration: 0.5 }}
            className="text-center"
          >
            <div className="animate-spin h-12 w-12 border-4 border-blue-500 border-t-transparent rounded-full mx-auto mb-4"></div>
            <p className="text-gray-400 text-lg">Loading nutrition data...</p>
          </motion.div>
        </CardContent>
      </Card>
    )
  }



  if (!nutritionData) {
    return (
      <Card className="w-full h-[600px] bg-black/50 backdrop-blur-lg shadow-xl">
        <CardHeader className="pb-3">
          <CardTitle className="text-gray-100 flex items-center gap-2">
            <Sparkles className="h-5 w-5 scale-2" />
            <span>Nutrition Facts</span>
          </CardTitle>
        </CardHeader>
        <CardContent className="flex flex-col items-center justify-center h-full">
          <motion.div
            initial={{ opacity: 0, y: 10 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ duration: 0.5 }}
            className="text-center"
          >
            <Apple className="h-16 w-16 text-white mx-auto mb-4" />
            <p className="text-white text-lg">Connect ESP-cam to see nutrition data</p>
            <p className="text-white text-sm mt-2">We'll analyze it and show you the details</p>
          </motion.div>
        </CardContent>
      </Card>
    )
  }

  const { food, nutritional_facts_per_gram } = nutritionData
  const multiplier = weight || 0

  // Calculate total values based on weight
  const calories = nutritional_facts_per_gram.calories * multiplier
  const carbs = nutritional_facts_per_gram.carbohydrates?.total * multiplier
  const sugars = nutritional_facts_per_gram.carbohydrates?.sugars * multiplier
  const fiber = nutritional_facts_per_gram.carbohydrates?.dietary_fiber * multiplier
  const protein = nutritional_facts_per_gram.protein * multiplier
  const fat = nutritional_facts_per_gram.fat * multiplier
  const potassium = nutritional_facts_per_gram.potassium_mg * multiplier

  // Calculate percentages for the progress bars (based on a 2000 calorie diet)
  const caloriesPercent = (calories / 2000) * 100
  const carbsPercent = (carbs / 300) * 100 // Approx daily value
  const proteinPercent = (protein / 50) * 100 // Approx daily value
  const fatPercent = (fat / 65) * 100 // Approx daily value

  // Format the timestamp
  const formattedTime = new Date(nutritionData.timestamp).toLocaleTimeString([], {
    hour: "2-digit",
    minute: "2-digit",
  })

  return (
    <motion.div initial={{ opacity: 0, y: 20 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.5 }}>
      <Card className="w-full h-full backdrop-blur-sm shadow-xl overflow-hidden bg-black/50 backdrop-blur-lg shadow-xl">
        <CardHeader className="pb-2 relative">
          <div className="absolute -top-10 -right-10 w-32 h-32 bg-blue-500/20 rounded-full blur-3xl"></div>
          <div className="absolute -bottom-10 -left-10 w-32 h-32 bg-purple-500/20 rounded-full blur-3xl"></div>

          <div className="flex items-center justify-between">
            <CardTitle className="text-xl text-white flex items-center gap-2">
              <span className=" font-bold">
                {food}
              </span>
            </CardTitle>
            <motion.div
              className="flex items-center gap-1 bg-gray-700/50 px-3 py-1 rounded-full text-sm font-medium text-blue-300"
              whileHover={{ scale: 1.05 }}
              whileTap={{ scale: 0.95 }}
            >
              <Droplet className="h-3 w-3" />
              <span>{weight.toFixed(1)}g</span>
            </motion.div>
          </div>


        </CardHeader>

        <AnimatePresence>
          {expanded && (
            <motion.div
              initial={{ height: 0, opacity: 0 }}
              animate={{ height: "auto", opacity: 1 }}
              exit={{ height: 0, opacity: 0 }}
              transition={{ duration: 0.3 }}
            >
              <CardContent className="pt-4">
                <div className="space-y-5">
                  {/* Calories */}
                  <motion.div
                    className="bg-gray-800/50 rounded-xl p-3 hover:bg-gray-800/80 transition-colors"
                    whileHover={{ y: -2 }}
                    transition={{ type: "spring", stiffness: 500 }}
                  >
                    <div className="flex justify-between items-center mb-2">
                      <div className="flex items-center gap-2">
                        <div className="bg-gradient-to-br from-orange-400 to-red-500 p-1.5 rounded-lg">
                          <Flame className="h-4 w-4 text-white" />
                        </div>
                        <span className="font-medium text-gray-200">Calories</span>
                      </div>
                      <span className="text-orange-300 font-bold">{calories.toFixed(1)} kcal</span>
                    </div>
                    <Progress
                      value={Math.min(caloriesPercent, 100)}
                      className="h-1.5 bg-gray-700"
                      indicatorClassName="bg-gradient-to-r from-orange-400 to-red-500"
                    />
                    <div className="mt-1 text-xs text-gray-400 text-right">
                      {caloriesPercent.toFixed(1)}% of daily value
                    </div>
                  </motion.div>

                  {/* Carbs */}
                  <motion.div
                    className="bg-gray-800/50 rounded-xl p-3 hover:bg-gray-800/80 transition-colors"
                    whileHover={{ y: -2 }}
                    transition={{ type: "spring", stiffness: 500 }}
                  >
                    <div className="flex justify-between items-center mb-2">
                      <div className="flex items-center gap-2">
                        <div className="bg-gradient-to-br from-yellow-400 to-amber-500 p-1.5 rounded-lg">
                          <Wheat className="h-4 w-4 text-white" />
                        </div>
                        <span className="font-medium text-gray-200">Carbohydrates</span>
                      </div>
                      <span className="text-yellow-300 font-bold">{carbs.toFixed(1)}g</span>
                    </div>
                    <Progress
                      value={Math.min(carbsPercent, 100)}
                      className="h-1.5 bg-gray-700"
                      indicatorClassName="bg-gradient-to-r from-yellow-400 to-amber-500"
                    />

                    <div className="grid grid-cols-2 gap-2 mt-3 text-sm">
                      <div className="flex items-center gap-2">
                        <div className="bg-gray-700 p-1 rounded-md">
                          <Cookie className="h-3 w-3 text-amber-300" />
                        </div>
                        <div className="flex items-center gap-1">
                          <span className="text-gray-400 text-xs">Sugars:</span>
                          <span className="text-amber-300 text-xs font-medium">{sugars.toFixed(1)}g</span>
                        </div>
                      </div>

                      <div className="flex items-center gap-2">
                        <div className="bg-gray-700 p-1 rounded-md">
                          <Salad className="h-3 w-3 text-green-300" />
                        </div>
                        <div className="flex items-center gap-1">
                          <span className="text-gray-400 text-xs">Fiber:</span>
                          <span className="text-green-300 text-xs font-medium">{fiber.toFixed(1)}g</span>
                        </div>
                      </div>
                    </div>
                  </motion.div>

                  {/* Protein */}
                  <motion.div
                    className="bg-gray-800/50 rounded-xl p-3 hover:bg-gray-800/80 transition-colors"
                    whileHover={{ y: -2 }}
                    transition={{ type: "spring", stiffness: 500 }}
                  >
                    <div className="flex justify-between items-center mb-2">
                      <div className="flex items-center gap-2">
                        <div className="bg-gradient-to-br from-purple-400 to-indigo-500 p-1.5 rounded-lg">
                          <Beef className="h-4 w-4 text-white" />
                        </div>
                        <span className="font-medium text-gray-200">Protein</span>
                      </div>
                      <span className="text-purple-300 font-bold">{protein.toFixed(1)}g</span>
                    </div>
                    <Progress
                      value={Math.min(proteinPercent, 100)}
                      className="h-1.5 bg-gray-700"
                      indicatorClassName="bg-gradient-to-r from-purple-400 to-indigo-500"
                    />
                  </motion.div>

                  {/* Fat */}
                  <motion.div
                    className="bg-gray-800/50 rounded-xl p-3 hover:bg-gray-800/80 transition-colors"
                    whileHover={{ y: -2 }}
                    transition={{ type: "spring", stiffness: 500 }}
                  >
                    <div className="flex justify-between items-center mb-2">
                      <div className="flex items-center gap-2">
                        <div className="bg-gradient-to-br from-blue-400 to-cyan-500 p-1.5 rounded-lg">
                          <Droplet className="h-4 w-4 text-white" />
                        </div>
                        <span className="font-medium text-gray-200">Fat</span>
                      </div>
                      <span className="text-blue-300 font-bold">{fat.toFixed(1)}g</span>
                    </div>
                    <Progress
                      value={Math.min(fatPercent, 100)}
                      className="h-1.5 bg-gray-700"
                      indicatorClassName="bg-gradient-to-r from-blue-400 to-cyan-500"
                    />
                  </motion.div>

                  {/* Additional Nutrients */}
                  <div className="bg-gray-800/30 rounded-xl p-3 mt-4">
                    <div className="text-sm text-gray-300 font-medium mb-2">Additional Nutrients</div>
                    <div className="grid grid-cols-2 gap-3 text-xs">
                      <div className="flex items-center gap-2">
                        <div className="bg-gray-700 p-1.5 rounded-lg">
                          <Sparkles className="h-3 w-3 text-yellow-300" />
                        </div>
                        <div>
                          <span className="text-gray-400">Vitamin C:</span>
                          <span className="text-yellow-300 ml-1 font-medium">
                            {nutritional_facts_per_gram.vitamin_c}
                          </span>
                        </div>
                      </div>
                      <div className="flex items-center gap-2">
                        <div className="bg-gray-700 p-1.5 rounded-lg">
                          <Sparkles className="h-3 w-3 text-green-300" />
                        </div>
                        <div>
                          <span className="text-gray-400">Potassium:</span>
                          <span className="text-green-300 ml-1 font-medium">{potassium.toFixed(1)}mg</span>
                        </div>
                      </div>
                      <div className="flex items-center gap-2 col-span-2">
                        <div className="bg-gray-700 p-1.5 rounded-lg">
                          <Droplet className="h-3 w-3 text-blue-300" />
                        </div>
                        <div>
                          <span className="text-gray-400">Water Content:</span>
                          <span className="text-blue-300 ml-1 font-medium">
                            {nutritional_facts_per_gram.water_content}
                          </span>
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </CardContent>
            </motion.div>
          )}
        </AnimatePresence>
      </Card>
    </motion.div>
  )
}
