"use client"

import { useState, useEffect } from "react"
import { database, ref, onValue } from "@/lib/firebase"

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

export function useFoodData() {
  const [foodData, setFoodData] = useState<NutritionData | null>(null)
  const [loading, setLoading] = useState<boolean>(true)
  const [error, setError] = useState<string | null>(null)
  const [isDataValid, setIsDataValid] = useState<boolean>(false)

  useEffect(() => {
    const foodRef = ref(database, "food/")

    try {
      const unsubscribe = onValue(
        foodRef,
        (snapshot) => {
          const data = snapshot.val() as NutritionData | null
          setFoodData(data)

          // Check if data exists and is less than 30 minutes old
          if (data && data.timestamp) {
            const thirtyMinutesInMs = 30 * 60 * 1000
            const isRecent = Date.now() - data.timestamp < thirtyMinutesInMs
            setIsDataValid(isRecent)
          } else {
            setIsDataValid(false)
          }

          setLoading(false)
        },
        (error) => {
          setError(error.message)
          setLoading(false)
        },
      )

      return () => unsubscribe()
    } catch (error) {
      setError("Failed to connect to Firebase")
      setLoading(false)
    }
  }, [])

  return { foodData, isDataValid, loading, error }
}
