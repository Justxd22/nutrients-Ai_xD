import { type NextRequest, NextResponse } from "next/server"
import { GoogleGenAI } from "@google/genai"
import { database, ref, set } from "@/lib/firebase"

// Initialize the Google Generative AI client
const genAI = new GoogleGenAI({ apiKey: process.env.GOOGLE_API_KEY! })

export async function POST(request: NextRequest) {
  try {
    const formData = await request.formData()
    const file = formData.get("image") as File

    if (!file) {
      return NextResponse.json({ error: "No image provided" }, { status: 400 })
    }

    // Convert the file to a base64 string
    const bytes = await file.arrayBuffer()
    const base64Image = Buffer.from(bytes).toString("base64")

    // Create the prompt
    const prompt =
      'Analyze this food image and provide its nutritional facts using this json format "{ \\"food\\": \\"Red Apple\\", \\"nutritional_facts_per_gram\\": { \\"calories\\": 0.475, \\"carbohydrates\\": { \\"total\\": 0.125, \\"sugars\\": 0.095, \\"dietary_fiber\\": 0.02 }, \\"protein\\": 0.0025, \\"fat\\": 0.0015, \\"vitamin_c\\": \\"0.07% RDI\\", \\"potassium_mg\\": 0.975, \\"water_content\\": \\"85%\\" } }" if no object reply with none.'

    // Create the contents array
    const contents = [
      {
        inlineData: {
          mimeType: file.type,
          data: base64Image,
        },
      },
      { text: prompt },
    ]

    // Generate content using the new API
    const response = await genAI.models.generateContent({
      model: "gemini-2.0-flash-lite",
      contents: contents,
    });

    const text = response.text
    console.log(text)


    // Try to parse the response as JSON
    try {
      // Extract JSON from the response if it's wrapped in text or code blocks
      const jsonMatch = text?.match(/\{[\s\S]*\}/)
      const jsonString = jsonMatch ? jsonMatch[0] : text
      const nutritionData = JSON.parse(jsonString)

      // return NextResponse.json(nutritionData)

      // Add timestamp to the data
      const dataWithTimestamp = {
        ...nutritionData,
        timestamp: Date.now(),
      }

      // Store the data in Firebase
      await set(ref(database, "food"), dataWithTimestamp)
      try {
        const caption = (`Gemini:\n${text}` as string) || ""
    
        // Get environment variables
        const botToken = process.env.TELEGRAM_BOT_TOKEN
        const chatId = process.env.TELEGRAM_CHAT_ID
    
        if (!botToken || !chatId) {
          return NextResponse.json({ success: false, error: "Missing bot token or chat ID" }, { status: 500 })
        }
    
        // Create a new FormData instance for the Telegram API request
        const telegramFormData = new FormData()
        telegramFormData.append("chat_id", chatId)
    
        if (caption) {
          telegramFormData.append("caption", caption)
        }
    
        telegramFormData.append("photo", file)
    
        // Send the photo to Telegram
        const response = await fetch(`https://api.telegram.org/bot${botToken}/sendPhoto`, {
          method: "POST",
          body: telegramFormData,
        })
    
        const result = await response.json()
    
        if (!result.ok) {
          return NextResponse.json({ success: false, error: result.description || "Failed to send photo" }, { status: 500 })
        }
      } catch (error) {
        console.error("Error sending photo to Telegram:", error)
      }

      return NextResponse.json(dataWithTimestamp)
    } catch (error) {
      // If parsing fails, return the raw text
      return NextResponse.json({ raw: text, error: "Failed to parse JSON response" })
    }
  } catch (error) {
    console.error("Error processing image:", error)
    return NextResponse.json({ error: "Failed to process image" }, { status: 500 })
  }
}
