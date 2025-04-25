"use client"

import { useState, useEffect } from "react"
import { database, ref, onValue } from "@/lib/firebase"

export function useScaleWeight() {
  const [weight, setWeight] = useState<number>(0)
  const [loading, setLoading] = useState<boolean>(true)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    const scaleRef = ref(database, "scale/weight")

    try {
      const unsubscribe = onValue(
        scaleRef,
        (snapshot) => {
          const data = snapshot.val()
          setWeight(data || 0)
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

  return { weight, loading, error }
}
